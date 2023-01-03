// expand_classes.cpp
// Created by Tein van der Lugt on 07/03/2022.

#include <iostream>
#include <set>
#include <cassert>
#include <cstring>
#include <sstream>
#include <algorithm>
#include "input.h"
#include "row.h"
#include "maps.h"
#include "matrix.h"
#include "algorithm_classes.h"
#include "input_detection.h"
#include "tags.h"


using namespace std;
using namespace panda;

string row_to_string(const Row<int> &row);

Matrix<int> reduce(Matrix<int> &rows, const Maps &maps, const OperationMode &mode, bool printClasses, bool printVerbose, bool printProgress);

void expand(Matrix<int> &rows, const Maps &maps, const OperationMode &mode, bool printExpandedRows, bool printVerbose);

tuple<Matrix<int>, Maps, OperationMode> read_input_data(int argc, char *argv[]);

Matrix<int> expandAndReduce(Matrix<int> &rows, const Maps &expandMaps, const Maps &reduceMaps, const OperationMode &mode, bool printVerbose);


int main(int argc, char *argv[]) {
    assert(argc >= 2);
    if (strcmp(argv[1], "--reduce") == 0) {
        tuple<Matrix<int>, Maps, OperationMode> data = read_input_data(argc, argv);
        Matrix<int> &rows = get<0>(data);
        const Maps &maps = get<1>(data);
        const OperationMode &mode = get<2>(data);

        reduce(rows, maps, mode, true, false, true);
    } else if (strcmp(argv[1], "--expand") == 0) {
        tuple<Matrix<int>, Maps, OperationMode> data = read_input_data(argc, argv);
        Matrix<int> &rows = get<0>(data);
        const Maps &maps = get<1>(data);
        const OperationMode &mode = get<2>(data);

        expand(rows, maps, mode, true, false);
    } else if (strcmp(argv[1], "--expand-and-reduce") == 0) {
        tuple<Matrix<int>, Maps, OperationMode> data = read_input_data(argc, argv);
        Matrix<int> &rows = get<0>(data);
        const Maps &maps = get<1>(data);
        const OperationMode &mode = get<2>(data);

        const Maps &expandMaps = {maps.begin(), maps.begin() + 7};
        const Maps &reduceMaps = {maps.begin() + 7, maps.end()};

        expandAndReduce(rows, expandMaps, reduceMaps, mode, true);
    } else {
        cout << "Please provide either the option '--reduce' or '--expand' or '--reduce-and-expand'." << endl;
        return 1;
    }
    return 0;
}


tuple<Matrix<int>, Maps, OperationMode> read_input_data(int argc, char *argv[]) {
    const auto mode = detectOperationMode(argc, argv);
    tuple<Matrix<int>, Names, Maps, Matrix<int>> data;
    if (mode == OperationMode::FacetEnumeration) {
        // The input file contains a 'Vertices:' section
        data = input::vertices<int>(argc, argv);
    } else if (mode == OperationMode::VertexEnumeration) {
        data = input::inequalities<int>(argc, argv);
    } else {
        cerr << "invalid operation mode!" << endl;
        abort();
    }

    Matrix<int> &rows = get<0>(data);
    const Maps &maps = get<2>(data);

    cerr << rows.size() << " rows found in input file." << endl;
    if (mode == OperationMode::VertexEnumeration) {
        cerr << "Ignoring final inequality " << row_to_string(rows.back()) << endl;
        rows.pop_back(); // Because for some reason they add a useless inequality at the end (see input::inequalities, in line with 'emplace_back').
    }
    cerr << endl;

    return make_tuple(rows, maps, mode);
}


Matrix<int> reduce(Matrix<int> &rows, const Maps &maps, const OperationMode &mode, bool printClasses, bool printVerbose, bool printProgress) {
    Matrix<int> classes;
    vector<unsigned long> classSizes;
    while (!rows.empty()) {
        set<Row<int>> row_class;
        if (mode == OperationMode::FacetEnumeration) {
            row_class = algorithm::getClass(*rows.begin(), maps, tag::vertex{});
        } else {
            row_class = algorithm::getClass(*rows.begin(), maps, tag::facet{});
        }
        assert(!row_class.empty());
        classes.push_back(*row_class.crbegin()); // Important detail: last element is chosen as the representative
        classSizes.push_back(row_class.size());

        // Clean up 'rows' so that we won't do double work
        for (const auto &row: row_class) {
            auto position = find(rows.begin(), rows.end(), row);
            while (position != rows.end()) {
                rows.erase(position);
                position = find(rows.begin(), rows.end(), row);
            }
        }

        // Output info
        if (printClasses) {
            cout << row_to_string(classes.back()) << ", class size " << classSizes.back() << endl;

            if (printVerbose) {
                cout << "has class" << endl;
                for (const Row<int> &row: row_class) {
                    cout << "@" << row_to_string(row) << endl;
                }
                cout << endl;
            }
        }

        // Print progress message
        if (printProgress) {
            cerr << "-----Vertices left to reduce: " << rows.size() << "     \r" << flush;
        }
    }

    cerr << "\nDone" << endl;

    return classes;
}


void expand(Matrix<int> &rows, const Maps &maps, const OperationMode &mode, bool printExpandedRows, bool printVerbose) {
//    set<Row<int>> class_reps;  // Keep track of class representatives; don't want to print classes twice.
    int i = 0;
    while (!rows.empty()) {
        cerr << "Expanding row " << ++i << ", " << rows.size() << " left\r" << flush;

        Row<int> row = *rows.begin();
        set<Row<int>> row_class;
        if (mode == OperationMode::FacetEnumeration) {
            row_class = algorithm::getClass(row, maps, tag::vertex{});
        } else {
            row_class = algorithm::getClass(row, maps, tag::facet{});
        }

//        class_reps.insert(*row_class.crbegin());
        // Remove all overlap between rows and row_class from rows:
        for (const Row<int> &_row: row_class) {
            const auto position = find(rows.begin(), rows.end(), _row);
            if (position != rows.end()) {
                rows.erase(position);
            }
        }

        if (printExpandedRows and printVerbose) {
            cout << "Point #" << i << '\n' << row_to_string(row) << "\nhas equiv class of size " << row_class.size() << " which is given by" << '\n';
        }
        if (printExpandedRows) {
            for (const Row<int> &_row: row_class) {
                cout << row_to_string(_row) << endl;
            }
        }
        cout << endl;
    }
    cerr << endl;
}

Matrix<int> expandAndReduce(Matrix<int> &rows, const Maps &expandMaps, const Maps &reduceMaps, const OperationMode &mode, bool printVerbose) {
    // Expand each row in rows by expandMaps, then reduce by reduceMaps. Do this for each row, then collect all resulting rows and finally, reduce all those by reduceMaps.
    Matrix<int> preresult;
    for (int i = 0; i < rows.size(); ++i) {
        cerr << "-------------------------------------------------------------------------Processing row " << i << " / " << rows.size() << "\r" << flush;

        Row<int> &row = rows.at(i);

        // Expand row using expandMaps
        set<Row<int>> row_class;
        if (mode == OperationMode::FacetEnumeration) {
            row_class = algorithm::getClass(row, expandMaps, tag::vertex{});
        } else {
            row_class = algorithm::getClass(row, expandMaps, tag::facet{});
        }

        // Convert row_class from set<Row> to vector<Row>, which is the same as Matrix<int>
        Matrix<int> row_class_matrix(row_class.begin(), row_class.end());     // the Matrix<int>(begin, end) constructor takes the beginning and end pointers of a set as arguments

        // Reduce row_class
        Matrix<int> row_class_reduced = reduce(row_class_matrix, reduceMaps, mode, false, false, true);

        // Append all rows in row_class_reduced to preresult
        preresult.insert(preresult.end(), row_class_reduced.begin(), row_class_reduced.end());

        // Output
        if (printVerbose) {
            cout << "Row #" << i << ":\n" << row_to_string(row) << "\nexpanded and reduced gives the following " << row_class_reduced.size() << " rows:\n";
            for (Row<int> &_row: row_class_reduced) {
                cout << "+" << row_to_string(_row) << endl;
            }
            cout << endl;
        }
    }

    // Finally, reduce preresult using reduceMaps
    cerr << "Doing final reduction..." << endl;
    Matrix<int> result = reduce(preresult, reduceMaps, mode, false, false, true);

    // Sort the resulting class representatives by loading them into a set (rather than vector)
    cerr << "Sorting..." << endl;
    set<Row<int>> sorted(result.begin(), result.end());
    cerr << "Done. Operation resulted in the following " << sorted.size() << " class representatives:\n";
    for (const Row<int> &row: sorted) {
        cout << row_to_string(row) << "\n";
    }
    return result;
}

/*void program_findLinIndepSubset(int argc, char *argv[], bool dataIsReduced) {
    if (!dataIsReduced) {
        cout << "dataIsReduced=false is not supported yet" << endl;
    }

    auto data = input::vertices<int>(argc, argv);  // a tuple with all the needed data
    const Matrix<int> rows = get<0>(data);
    const Maps maps = get<2>(data);

    for (int i = 0; i < rows.size(); ++i) {
        cout << "Processing class #" << i << ", " << rows.size() << " classes left to process.";

        const set<Row<int>> row_class = algorithm::getClass(*rows.begin(), maps, tag::vertex{});
        const set<Row<int>> linIndepSubset = findLinIndepSubset(row_class);


        // Ensure that we'll not process this class again later
        for (const auto &row: row_class) {
            const auto position = find(rows.begin(), rows.end(), row);
            if (position != rows.end()) {
                rows.erase(position);
            }
        }
    }
}*/

/*set<Row<int>> findLinIndepSubset(Matrix<int>& rows) {
    int i = 0;
    while (i < rows.size()) {
        arma::Mat<double> matrix;
    }

    for (const Row<int>& row: rows) {
        // Create matrix of result + row
        // check if matrix has full rank
        // if so, add row to result
        // arma::Mat<int> matrix = rows_to_arma_matrix()
    }
}*/

string row_to_string(const Row<int> &row) {
    stringstream stream;
    for (int i: row) {
        stream << i << " ";
    }
    return stream.str();
}