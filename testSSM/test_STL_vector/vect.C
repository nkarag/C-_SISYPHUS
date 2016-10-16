#include <iostream>
#include <vector>

int main() {
	
	vector< vector<int> > vvint;

	vector<int> v1(50, 1);
	vector<int> v2(150, 2);
	vector<int> v3(10, 3);
	vector<int> v4(100, 4);

	vvint.push_back(v1);
	vvint.push_back(v2);
	vvint.push_back(v3);
	vvint.push_back(v4);

	vector<int>& out1 = vvint[0];
	cout<<"out1[50-1] = "<<out1[50-1]<<endl;

	vector<int>& out2 = vvint[1];
	cout<<"out2[150-1] = "<<out2[150-1]<<endl;
	
	vector<int>& out3 = vvint[2];
	cout<<"out3[10-1] = "<<out3[10-1]<<endl;
	
	vector<int>& out4 = vvint[3];
	cout<<"out4[100-1] = "<<out4[100-1]<<endl;
}
