class Callback : public GRBCallback
{
private:
	/*
		Variables
	 */
	vector<vector<GRBVar> > kb, a;
	GRBVar vl;

    int sizeAlphabet;
    bool finished;

	vector<vector<bool> > XOR1Added, XOR2Added, XOR3Added, XOR4Added;

	vector<int> sl;

	vector<GRBLinExpr> li;
	vector<GRBLinExpr> lj;

	vector<vector<GRBLinExpr> > constraints_XOR1;
	vector<vector<GRBLinExpr> > constraints_XOR2;
	vector<vector<GRBLinExpr> > constraints_XOR3;
	vector<vector<GRBLinExpr> > constraints_XOR4;

	/*
		Methods
	 */
	bool isSatisfied(string constr, int i, int j){ 
		if(constr == "XOR1"){
			int sum = 0;
			for (int k = 0; k < sizeAlphabet; ++k)
				sum += ((getSolution(kb[k][i]) + getSolution(kb[k][j])) * sl[k]);
				
			return getSolution(a[i][j]) <= sum;
		}

		else if(constr == "XOR2"){
			int sum = 0;
			for (int k = 0; k < sizeAlphabet; ++k)
				sum += ((getSolution(kb[k][i]) - getSolution(kb[k][j])) * sl[k]);
			
			return getSolution(a[i][j]) >= sum;
		}

		else if(constr == "XOR3"){
			int sum = 0;
			for (int k = 0; k < sizeAlphabet; ++k)
				sum += ((getSolution(kb[k][j]) - getSolution(kb[k][i])) * sl[k]);
			
			return getSolution(a[i][j]) >= sum;
		}

		else if(constr == "XOR4"){
			int sum = 0;
			for (int k = 0; k < sizeAlphabet; ++k)
				sum += ((getSolution(kb[k][i]) + getSolution(kb[k][j])) * sl[k]);
			
			return getSolution(a[i][j]) <= 2 - sum;
		}

		return false;
	}


public:
	/*
		Constructor
	 */
	Callback(int _sizeAlphabet, vector<vector<GRBVar> > _kb, GRBVar _vl, vector<vector<GRBVar> > _a, vector<int> _sl) : 
			finished(false), sl(_sl), kb(_kb), vl(_vl), a(_a), sizeAlphabet(_sizeAlphabet),
			XOR1Added(_sizeAlphabet, vector<bool>(_sizeAlphabet,false)), XOR2Added(_sizeAlphabet, vector<bool>(_sizeAlphabet,false)), XOR3Added(_sizeAlphabet, vector<bool>(_sizeAlphabet,false)), XOR4Added(_sizeAlphabet, vector<bool>(_sizeAlphabet,false))
	{

		li = vector<GRBLinExpr>(_sizeAlphabet);
		for (int i = 0; i < _sizeAlphabet; ++i)
			for (int k = 0; k < _sizeAlphabet; ++k)
				li[i] += (_kb[k][i] * _sl[k]);

		lj = vector<GRBLinExpr>(_sizeAlphabet);
		for (int j = 0; j < _sizeAlphabet; ++j)
			for (int k = 0; k < _sizeAlphabet; ++k)
				lj[j] += (_kb[k][j] * _sl[k]);

		constraints_XOR1 = vector<vector<GRBLinExpr> >(sizeAlphabet, vector<GRBLinExpr>(sizeAlphabet));
		constraints_XOR2 = vector<vector<GRBLinExpr> >(sizeAlphabet, vector<GRBLinExpr>(sizeAlphabet));
		constraints_XOR3 = vector<vector<GRBLinExpr> >(sizeAlphabet, vector<GRBLinExpr>(sizeAlphabet));
		constraints_XOR4 = vector<vector<GRBLinExpr> >(sizeAlphabet, vector<GRBLinExpr>(sizeAlphabet));

		for (int i = 0; i < _sizeAlphabet; ++i)
		{ 
			for (int j = 0; j < _sizeAlphabet; ++j)
			{
				constraints_XOR1[i][j] = li[i] + lj[j];
				constraints_XOR2[i][j] = li[i] - lj[j];
				constraints_XOR3[i][j] = lj[j] - li[i];
				constraints_XOR4[i][j] = 2 - li[i] - lj[j];
			}
		}

	}

protected: 
	void callback(){
		try{
			if (where == GRB_CB_MIPSOL)
			{
				if(!finished){
					int countAdded = 0;
					bool haveToAddMore = false;
					for (int i = 0; i < sizeAlphabet && countAdded < 36; ++i)
					{
						for (int j = 0; j < sizeAlphabet && countAdded < 36; ++j)
						{
							if(!XOR1Added[i][j]){
								if(!isSatisfied("XOR1", i, j)){
									addLazy(a[i][j] <= constraints_XOR1[i][j]);
									XOR1Added[i][j] = true;
									XOR1Added[j][i] = true;
									countAdded++;
									haveToAddMore = true;
								}
							}

							if(!XOR2Added[i][j]){
								if(!isSatisfied("XOR2", i, j)){
									addLazy(a[i][j] >= constraints_XOR2[i][j]);
									XOR2Added[i][j] = true;
									countAdded++;
									haveToAddMore = true;
								}
							}

							if(!XOR3Added[i][j]){
								if(!isSatisfied("XOR3", i, j)){
									addLazy(a[i][j] >= constraints_XOR3[i][j]);
									XOR3Added[i][j] = true;
									countAdded++;
									haveToAddMore = true;
								}
							}

							if(!XOR4Added[i][j]){
								if(!isSatisfied("XOR4", i, j)){
									addLazy(a[i][j] <= constraints_XOR4[i][j]);
									XOR4Added[i][j] = true;
									XOR4Added[j][i] = true;
									countAdded++;
									haveToAddMore = true;
								}
							}
						}
					}

					if(!haveToAddMore){
						finished = true;
						cout << "No lazy contraint to add anymore" << endl;
					}
				}
			}
			
		} catch (GRBException e){
			cout << "Error number: " << e.getErrorCode() << endl;
			cout << "Message erreur : " << e.getMessage() << endl;
		} catch (...){
			cout << "Error during callback." << endl;
		}
		
	}
	
};
