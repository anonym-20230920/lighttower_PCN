#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>
#include <map>
#include <utility>
using namespace std;

int main() {
	for(int d = 2; d <= 5; d++)
		for(double gamma = 0; gamma < 1.01; gamma += 0.05) {
			double ave_sum = 0.0;
			double dia_sum = 0.0;
			double cut_sum = 0.0;
			for(int it = 0; it < 3; it++){
				FILE *fin = fopen("in.txt", "w");
				fprintf(fin, "%d %.2f", d, gamma);
	//			cout <<d <<' '<<gamma <<endl;
				fclose(fin);
				system("./simu <in.txt>out.txt");
				FILE *fres = fopen("out.txt", "r");
				double ave, dia, cut;
				fscanf(fres, "%lf %lf %lf", &ave, &dia, &cut);
				ave_sum += ave;
				dia_sum += dia;
				cut_sum += cut;
			}
			ave_sum /= 3.0;
			dia_sum /= 3.0;
			cut_sum /= 3.0;
			cout <<d <<',' <<gamma <<','<<ave_sum <<','<<dia_sum <<','<<cut_sum <<endl;
		}
}
