#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>
#include <map>
#include <utility>
#include <climits>
#include <ctime>
#include <cassert>
#include <iterator>
using namespace std;

const int MAXV = 1000000;
const int MAXE = MAXV * (log(MAXV) / log(2)) *3;

const double randgraph_param = 0.6;

struct PCN {
	int num;
	int edge_count;
	vector<int> adj[MAXV]; 	// this is the vector of edgeIndex, not vector of vertices
	int cap[MAXE];
	int flow[MAXE];
	int sender[MAXE];
	int receiver[MAXE];
	int rev[MAXE]; // the reverse edge, for maxflow only
	bool is_rev[MAXE];
};

int powers[100];

int power(int len, int d) {
	int prod = 1;
	while(d--) prod *= len;
	return prod;
}

inline int randN() {	// returns a random uint of 30 bits
	return ((rand()&0x7fff)<<15) + (rand()&0x7fff);
}

int index_to_number(const vector<int> &index, int len, int d) {
	int res = 0;
	for(int i = 0; i < d; i++)
		res = res + index[i] * powers[d - i - 1];
	return res;
}

vector<int> number_to_index(int X, int len, int d) {
	vector<int> index;
	index.clear();
	for(int i = 0; i < d; i++)
		index.push_back((X / powers[d - i - 1]) % len);
	return index;
}

void connect(PCN &pcn, const vector<int> ia, const vector<int> ib, int len, int d, int w) {
 	int a = index_to_number(ia, len, d);
 	int b = index_to_number(ib, len, d);
 	int ct = ++ pcn.edge_count;
 	pcn.sender[ct] = a;
 	pcn.receiver[ct] = b;
 	pcn.cap[ct] = w;
	pcn.flow[ct] = 0;
	pcn.adj[a].push_back(ct);
}

void connect2(PCN &pcn, int a, int b, int len, int d, int w) {
 	int ct = ++ pcn.edge_count;
 	pcn.sender[ct] = a;
 	pcn.receiver[ct] = b;
 	pcn.cap[ct] = w;
	pcn.flow[ct] = 0;
	pcn.adj[a].push_back(ct);
}

double rand01() {
	return rand() / (double)RAND_MAX;
}

void build_PCN(PCN &pcn, int len, int d, int weight, double gamma) {
	pcn.num = int(pow(len,d) + 0.001);

	pcn.edge_count = 0;

	int deglimit = (int(log(len) / log(2)) + 2) * d * gamma;
	int degremain = (int(log(len) / log(2)) + 2) * d  -  deglimit;

//	cerr << "deglimit = " <<deglimit <<"; degremain = " <<degremain <<endl;

	for(int i = 0; i < pcn.num; i++) {
		vector<int> index = number_to_index(i, len, d);
		pcn.adj[i].clear();
		for(int k = 0; k < d; k++) {
			vector<int> indexTo = index;	// this is deep clone for C++
			if(pcn.adj[i].size() >= deglimit) 
				break;
			indexTo[k] = (index[k] + 1) % len;
			connect(pcn, index, indexTo, len, d, weight);
			if(pcn.adj[i].size() >= deglimit) 
				break;
			indexTo[k] = (index[k] + len - 1) % len;
			connect(pcn, index, indexTo, len, d, weight);
		}
		for(int step = 2; step < len; step *= 2) {
			for(int k = 0; k < d; k++) {
				vector<int> indexTo = index;
				if(pcn.adj[i].size() >= deglimit) 
					break;
				indexTo[k] = (index[k] + step) % len;
				connect(pcn, index, indexTo, len, d, weight);
			}
		}
	}

	connect2(pcn, 0, 0, len, d, weight);

	for(int i = 1; i < pcn.num; i++) {
		vector<int> index = number_to_index(i, len, d);
		for(int target, k = 0; k < degremain; k++) {
			if(rand01() < randgraph_param) // 1<->randgraph_param
				target = randN() % i; //n<->i
			else {
				vector<int> *pointer = &pcn.adj[randN() % i];
				target = pcn.receiver[(*pointer)[randN() % pointer->size()]];
				/*
				Notice: 
				(uniformly sampling a previous $u$ and sample a random $w$ that $u$ points to)
				IS EQUIVALNT TO
				(sampling a random $w$ with prob. prop. to indegree)
				*/
			}
			connect2(pcn, i, target, len, d, weight);
		}
	}
//	cerr <<"Building Complete!" <<endl;
}

double average_degree(PCN &pcn) { // provable: average_out_degree = average_in_degree
	int sum = 0;
	for(int i = 0; i < pcn.num; i++)
		sum = sum + (int) pcn.adj[i].size();
	return sum / (double)pcn.num;
}

int qu[MAXE*2];
int d[MAXV];

double average_distance(PCN &pcn, int VERTICES_SAMPLED_FOR_DISTANCE) {
	double sum = 0;
	int sample_num = 0;

	for(int iter = 1; iter < VERTICES_SAMPLED_FOR_DISTANCE; iter++) {
		int source = randN() % pcn.num;
		// BFS is sufficient for outputting shortest paths
		qu[0] = source;
		for(int i = 0; i < pcn.num; i++) 
			d[i] = (1<<30); //simulates infinity
		d[source] = 0;
		int *l = qu, *r = qu;
		for(;l <= r; l++) {
			int cur = *l;
			for(vector<int>::iterator it = pcn.adj[cur].begin(); it != pcn.adj[cur].end(); it++) 
				if(d[pcn.receiver[*it]] > d[cur] + 1) {
					*++r = pcn.receiver[*it];
					d[*r] = d[cur] + 1;
				}
		}
		assert(r-qu <= pcn.num);
		// BFS over
	
		for(int i = 0; i < pcn.num; i++)
			if(i != source) {
				if(d[i] == (1<<30))
					return -1.0;	//returning -1 means infinity (not strongly connected)
				sum += d[i];
				sample_num ++;
			}
	}
	return sum / (double) sample_num;
}

double esti_diameter(PCN &pcn, int ESTI_NUM) {
	double sum = 0;
	for(int est = 0; est < ESTI_NUM; est++) {
		int source = randN() % pcn.num;
		qu[0] = source;
		for(int i = 0; i < pcn.num; i++) 
			d[i] = (1<<30); //simulates infinity
		d[source] = 0;
		int *l = qu, *r = qu;
		for(;l <= r; l++) {
			int cur = *l;
			for(vector<int>::iterator it = pcn.adj[cur].begin(); it != pcn.adj[cur].end(); it++) 
				if(d[pcn.receiver[*it]] > d[cur] + 1) {
					*++r = pcn.receiver[*it];
					d[*r] = d[cur] + 1;
				}
		}
		assert(r-qu <= pcn.num);
		int maxd = 0;
		int posi = 0;
		for(int i = 0; i < pcn.num; i++)
			if(d[i] > maxd) {
				maxd = d[i];
				posi = i;
			}
		sum += maxd;
	}
	return sum >= (1<<30) ?-1 :sum / (double)ESTI_NUM;
}

void add_reserve_edges(PCN &pcn) {
	int existing = pcn.edge_count;
	for(int i = 0; i < existing; i++) {
		int a = pcn.sender[i];
		int b = pcn.receiver[i];

	 	int ct = ++ pcn.edge_count;
	 	pcn.sender[ct] = b;
	 	pcn.receiver[ct] = a;
	 	pcn.cap[ct] = 0;
		pcn.flow[ct] = 0;
		pcn.adj[b].push_back(ct);

		pcn.rev[i] = ct;
		pcn.rev[ct] = i;

		pcn.is_rev[i] = false;
		pcn.is_rev[ct] = true;
	}
}

void reset_cap(PCN &pcn) {
	for(int i = 0; i < pcn.edge_count; i++) {
		if(pcn.is_rev[i])
			pcn.cap[i] = 0;
		else 
			pcn.cap[i] = 1;
	}
}

int path[MAXV]; // array of edgeIndex
int pre[MAXV];


bool find_path(PCN &pcn, int source, int sink) {	//Dinic
	for(int i = 0; i < pcn.num; i++) {
		d[i] = -1;
	}
	qu[0] = source;
	pre[source] = -1;
	d[source] = 0;
	

	#define cost(e) (pcn.is_rev[e] ?-1 :1)

	int *l = qu, *r = qu;
	for(; l <= r; l++) {
	//	cerr<<*l<<endl;
		for(vector<int>::iterator it = pcn.adj[*l].begin(); it != pcn.adj[*l].end(); it++) 
			if(pcn.cap[*it]) {
				int u = pcn.receiver[*it];
				if(d[u] == -1) {
					d[u] = d[*l] + 1;
					*++r = u;
					pre[u] = *it;
				}
			}
	}

	if(d[sink] == -1)
		return false;

	int i = 0, j = sink;
	for(; pre[j] != -1; j = pcn.sender[pre[j]]) {
		path[i++] = pre[j];
	}
	assert(j == source);
	path[i] = -1;
	return true;
}

int maxflow(PCN &pcn, int source, int sink) {
	int sum = 0;
	int s;

	while(s = find_path(pcn, source, sink)) {
		sum += s;
	//	cerr <<s <<endl;
		for(int i = 0; path[i] != -1; i++) {
			int j = path[i];
			pcn.cap[j] -= s;	// s=1 anyway
			pcn.cap[pcn.rev[j]] += s;
		}
	}

	return sum;
}

double expected_mincut(PCN &pcn, int PAIR_SAMPLED) {
	add_reserve_edges(pcn);
	double sum = 0;
	for(int i = 0; i < PAIR_SAMPLED; i++) {
		int source = randN() % pcn.num;
		int sink = randN() % pcn.num;

		reset_cap(pcn);

		int res = maxflow(pcn, source, sink);
		sum += res;	
	}
	
	return sum / (double) PAIR_SAMPLED;
}

PCN pcn;

int main() {
	int d;
	double gamma;
	cin >>d >>gamma;
//	cerr <<d <<' '<<gamma <<endl;
	int len = 10;
	int weight = 1;
	powers[0] = 1;
	for(int i = 1; i < d; i++)
		powers[i] = powers[i-1] * len;
	srand(time(NULL));

//	freopen("out.txt","w",stdout);
	build_PCN(pcn, len, d, weight, gamma);
//	cerr <<pcn.num <<endl;
	cout <<average_distance(pcn, 30) <<' ';
	cout <<esti_diameter(pcn,30) <<' ';
	cout <<expected_mincut(pcn, 30) <<endl;;
//	cerr <<endl;
}
