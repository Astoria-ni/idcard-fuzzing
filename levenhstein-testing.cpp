#include "bits/stdc++.h"
using namespace std;

double solve(string s, string t){
	int n=s.length(),m=t.length();
	s='#'+s;
	t='#'+t;
	int d[n+5][m+5];
	d[0][0] = 0;
	for(int i=1; i<=n; i++) d[i][0] = i;
	for(int j=1; j<=m; j++) d[0][j] = j;
	
	for(int i=1; i<=n; i++){
		for(int j=1; j<=m; j++){
			int sc=1;
			if(s[i]==t[j]) sc=0;
			d[i][j] = min(d[i-1][j]+1, min(d[i][j-1]+1, d[i-1][j-1]+sc));
		}
	}
	int modif = d[n][m];
	
	double approx_diff = (double)modif / (double)(n+m) * 2.0;
	return approx_diff;
}

double find_levenhstein(vector<string> vec1, vector<string> vec2){
	assert(vec1.size() == vec2.size());
	double tot = 0.0;
	for(int i=0; i<vec1.size(); i++){
		tot += solve(vec1[i],vec2[i]);
	}
	tot /= ((double)(vec1.size()));
	return tot;
}
