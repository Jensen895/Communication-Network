#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>


using namespace std;

struct node{
    int backoff;
    int r_id;
    bool channelOccupied;
};

void readInput(char* argv1, int &N, int &L, int &M, int &T, vector<int> & R){
        // read input.txt
    ifstream inputFile(argv1);
    string line;
    while(getline(inputFile, line)){
        istringstream iss(line);
        char idx;
        int val;
        if (iss >> idx) {
            if (idx == 'N' && iss >> val) N = val;
            else if(idx == 'L' && iss >> val) L = val;
            else if(idx == 'M' && iss >> val) M = val;
            else if(idx == 'T' && iss >> val) T = val;
            else if(idx == 'R'){
                int value;
                while (iss >> value) {
                    R.push_back(value);
                }
            }
        }        

    }
    inputFile.close();
}

int main(int argc, char** argv){
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }

    /*
    N : number of nodes
    L : packet size
    M : maximum retransmission attempt
    R : the initial random number range
    T : total time of simulation
    */

    int N, L, M, T;
    vector<int> R;
    readInput(argv[1], N, L, M, T, R);
    cout << "input data loaded" << endl;

    node * nd = new node[N];
    for (int i = 0; i < N; ++i) {
        nd[i].r_id = 0;
        nd[i].backoff = (i+0)% R[nd[i].r_id]; //tick = 0
    }
    cout << "nodes' state initialized" << endl;
    
    ofstream outputFile("output.txt");

    int transmitted = 0;
    vector<int> cntZero;
    cntZero.push_back(0);
    for(int t = 0 ; t < T;  t++){
        cntZero.clear();
        for (int i = 0; i < N; i++) {
            if(nd[i].backoff == 0) cntZero.push_back(i);
        }
        if(cntZero.size() == 0){
            for (int i = 0; i < N; i++) nd[i].backoff--;
        }
        else if (cntZero.size() == 1){ //transmission
            int n_id = cntZero.back();
            // outputFile << n_id << " " << nd[n_id].r_id << " " << nd[n_id].backoff << endl;
            nd[n_id].r_id = 0;
            nd[n_id].backoff = (n_id + t + L) % R[ nd[n_id].r_id ];
            if(t + L < T){
                t += L - 1;
                transmitted += L;
            }
            else{
                transmitted += T - t;
                t = T - 1;
                // break;
            }
            cntZero.pop_back();

        }
        else{ //collision
            while(!cntZero.empty()){
                int n_id = cntZero.back();
                nd[n_id].r_id++;
                if(nd[n_id].r_id == M) nd[n_id].r_id = 0;
                nd[n_id].backoff = (n_id + t + 1) % R[nd[n_id].r_id];
                cntZero.pop_back();
            }

        }
    }


    float utilRate = (float)transmitted / T;
    // ofstream outputFile("output.txt");
    outputFile << fixed << setprecision(2) << utilRate << endl;
    outputFile.close();
    cout << "output file written successfully" << endl;

    

    return 0;
}