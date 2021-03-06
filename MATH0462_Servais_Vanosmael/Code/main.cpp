#include <algorithm>
#include <sstream>
#include <vector>
#include <cmath>

#include "gurobi_c++.h"
#include "parser.hpp"
#include "Callback.hpp"

using namespace std;


void frequencyZone (vector<vector<GRBVar> >& kb, vector<double>& fr, vector<int>& sl){
/* 	row 1 = 0 - 11
	row 2 = 12 - 23
	row 3 = 24 - 35
	row 4 = 36 - 46
*/	
	double rows[4] = {0.0, 0.0, 0.0, 0.0};
	double left = 0.0, right = 0.0;

	for(int k = 0; k < kb.size() ; ++k) {
		for(int l = 0; l < kb[k].size() ; ++l) {
			if((int)(kb[k][l].get(GRB_DoubleAttr_X) + 0.5) == 1){ //If we are on a winning k,l pair
				//Rows				
				int row = k / 12; //Except the last one each row are 12 keys long
				rows[row] += fr[l];
			
				//Hand
				if(sl[k]) 
					left += fr[l];
				else
					right += fr[l];
			}
		}
	}

	for(int i = 0;  i < 4; ++i)
		cout << "Row " << i << " : " << rows[i] *100 << "%" << endl;

	cout << "Left Hand : " << left *100 << "%" << endl;
	cout << "Right Hand : " << right *100 << "%" << endl;
}

string findChar (int i, vector<vector<GRBVar> >& kb, vector<string>& alphabet){
	for (int j = 0; j < alphabet.size() ; ++j)
		if((int)(kb[i][j].get(GRB_DoubleAttr_X) + 0.5) == 1)
			return alphabet[j];
		
	return " ";
}

void displayKeyboard (vector<vector<GRBVar> >& kb, vector<string> alphabet){
	for (int i = 0; i < 12; ++i)
		cout << findChar(i, kb, alphabet) << " ";
	cout << endl << " ";

	for (int i = 12; i < 24; ++i)
		cout << findChar(i, kb, alphabet) << " ";
	cout << endl << "  ";

	for (int i = 24; i < 36; ++i)
		cout << findChar(i, kb, alphabet) << " ";
	cout << endl;

	for (int i = 36; i < alphabet.size(); ++i)
		cout << findChar(i, kb, alphabet) << " ";
	cout << endl;
}





int main(int argc, char const *argv[])
{
	/**
	 * ============== Parsing ============== 
	 */
	const char* freq1File;
	const char* freq2File;	

	if(argc != 3){
		cout << "Usage : ./main <count of symbols file> <count of bigrams file>" << endl;
		return -1;	
	}

	freq1File = argv[1];
	freq2File = argv[2];

	// Datamodel m(<data file>, <count of symbols file>, <count of bigrams file>)
	DataModel m("data", freq1File, freq2File);
	if(m.isInvalid()){
		cerr << "ERROR : One or several input file(s) have failed to open." << endl;
		return -1;
	}

	int numberKeys = m.getNumberKeys();
	int sizeAlphabet = numberKeys;
	int numberVowels = m.getNumberVowels();

	vector<int> dk = m.getDistanceKey();
	vector<double> fr = m.getFrequencies();
	vector<double> ks = m.getStrength();
	vector<int> sl = m.getLeftHandKeys();
	vector<int> sr = m.getRightHandKeys();
	vector<int> v = m.getLeftSideLetters(); 

	vector<vector<double> > w = m.getBigramsFreq();

	/**
	 * ==============  Computation ============== 
	 */
	try{
		// Set new environment
		cout << "Creating environment" << endl;
		GRBEnv env = GRBEnv();

		// Create new model
		cout << "Creating model" << endl;
		GRBModel model = GRBModel(env);

		model.getEnv().set(GRB_IntParam_LazyConstraints, 1);

		/**
		 * Variables
		 */
		cout << "Creating variables" << endl;
		vector<vector<GRBVar> > kb(numberKeys, vector<GRBVar>(sizeAlphabet)); // Assignment of the letters with the keys
		GRBVar vl; // Vowel on the left side or not
		vector<vector<GRBVar> > a(numberKeys, vector<GRBVar>(sizeAlphabet)); // Not the same hand to type one then another

		// kb_{k,l} : Assign the letters to the key
		for (int k = 0; k < numberKeys; ++k){
			for (int l = 0; l < sizeAlphabet; ++l){
				stringstream ss;
				ss << "kb_" << k << "_" << l;
				kb[k][l] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, ss.str());
			}
		}

		// vl : side of the vowels
		vl = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "vl");

		// a : = 1 if not the same hand used to go from i to j
		for (int i = 0; i < sizeAlphabet; ++i){
			for (int j = 0; j < sizeAlphabet; ++j){
				stringstream ss;
				ss << "a_" << i << "_" << j;

				if (i == j)
					a[i][j] = model.addVar(0.0, 0.0, 0.0, GRB_BINARY, ss.str());
				else
					a[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, ss.str());
			}
		}

		model.update();

		/**
		 * Objective function
		 */
		cout << "Creating objective function" << endl;
		GRBLinExpr objFunction;

		// First term
		for (int i = 0; i < sizeAlphabet; ++i){
			GRBLinExpr term;

			for (int j = 0; j < numberKeys; ++j)
				term += (dk[j] * ks[j] * kb[j][i]);

			term *= fr[i];
			objFunction += term;
		}

		// Second term
		for (int i = 0; i < sizeAlphabet; ++i)
			for (int j = 0; j < sizeAlphabet; ++j)
				objFunction += (w[i][j] * (1 - a[i][j]));

		// Third term
		for (int i = 0; i < sizeAlphabet; ++i){
			for (int j = 0; j < sizeAlphabet; ++j){
				GRBLinExpr term;

				for (int k = 0; k < numberKeys; ++k)
					term += (dk[k] * kb[k][i]);

				for (int l = 0; l < numberKeys; ++l)
					term += (dk[l] * kb[l][j]);
				

				objFunction += (term * w[i][j]);
			}
		}

		model.setObjective(objFunction, GRB_MINIMIZE);

		/**  
		 * Contraints
		 */
		cout << "Creating constraints" << endl;

		GRBLinExpr side_constr;
		for (int l = 0; l < sizeAlphabet; ++l)
		{
			GRBLinExpr term;

			for (int k = 0; k < numberKeys; ++k)
				term += (kb[k][l] * (sr[k] - sl[k]));

			side_constr += (term * fr[l]);
		}
		model.addConstr(side_constr >= 0, "side_constraint");

		// One key per letter
		for (int l = 0; l < sizeAlphabet; ++l)
		{
			GRBLinExpr keyPerLetter;
			
			for (int k = 0; k < numberKeys; ++k)
				keyPerLetter += kb[k][l];

			stringstream ss;
			ss << "One key for letter " << l;
			model.addConstr(keyPerLetter == 1, ss.str());
		}

		// One letter per key
		for (int k = 0; k < numberKeys; ++k)
		{
			GRBLinExpr letterPerKey;
			
			for (int l = 0; l < sizeAlphabet; ++l)
				letterPerKey += kb[k][l];
			
			stringstream ss;
			ss << "One letter for key " << k;
			model.addConstr(letterPerKey == 1, ss.str());
		}

		// Vowels on the same hand
		GRBLinExpr vowels_side;
		for (int l = 0; l < sizeAlphabet; ++l)
		{
			GRBLinExpr term;

			for (int k = 0; k < numberKeys; ++k)
				term += (kb[k][l] * sl[k]);

			vowels_side += (term * v[l]);
		}
		model.addConstr(vowels_side == numberVowels * vl, "Vowels on the same hand");

		for (int i = 0; i < sizeAlphabet; ++i)
		{
			stringstream ss;
			ss << "a_" << i << "_" << i << " constraint";
			model.addConstr(a[i][i] == 0, ss.str());
		}

		for (int i = 0; i < sizeAlphabet; ++i)
		{
			for (int j = 0; j < sizeAlphabet; ++j)
			{
				stringstream ss;
				ss << "a_" << i << "_" << j << " constraint";
				model.addConstr(a[i][j] == a[j][i], ss.str());
			}
		}

		Callback cb(sizeAlphabet, kb, vl, a, sl);
		model.setCallback(&cb);

		model.optimize();

		/**
		 * Print
		 */
		
		/*
		cout << "======== kb ========" << endl; 
		for (int k = 0; k < numberKeys; ++k)
			for (int l = 0; l < sizeAlphabet; ++l) 
				cout << kb[k][l].get(GRB_StringAttr_VarName) << " = " << kb[k][l].get(GRB_DoubleAttr_X) << endl;
	
		cout << endl;
		
		cout << "======== vl ========" << endl;
		cout << vl.get(GRB_StringAttr_VarName) << " = " << vl.get(GRB_DoubleAttr_X) << endl;

		cout << endl;

		cout << "======== a ========" << endl;
		for (int i = 0; i < sizeAlphabet; ++i)
			for (int j = 0; j < sizeAlphabet; ++j) 
				cout << a[i][j].get(GRB_StringAttr_VarName) << " = " << a[i][j].get(GRB_DoubleAttr_X) << endl;
		
		cout << endl;

		*/
		
		cout << "Objective function = " << model.get(GRB_DoubleAttr_ObjVal) << endl;

		// Display keyboard
		cout << endl;
		
		displayKeyboard(kb, m.getAlphabet());

		cout << endl;

		cout << "Statistic results : " << endl;
		frequencyZone(kb, fr, sl);
		cout << endl; 

		cout << "Check if all constraints are satisfied : ";
		if (!cb.isSatisfied())
			cout << "No" << endl;
		else
			cout << "OK" << endl; 
		 
		cout << endl;


	} catch(GRBException e){
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	} catch(...){
		cout << "Exception during optimization" << endl;
	}


	
	return 0;
}
