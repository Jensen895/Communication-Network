#include <iostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <typeinfo>
#include <string>
#include <vector>
#include <climits>

#define s second
#define f first
#define vvi  vector<vector<int> > 
#define vvp  vector<vector<pair<int, int> > >

using namespace std;

int countVertices(char *argv1){
    cout << "countVertices..." << endl;
    ifstream topofile(argv1);
    string sourceId, destId, cost;
    int numV = 0;
    while(topofile >> sourceId >> destId >> cost){
        numV = max(numV, atoi(sourceId.c_str()));
        numV = max(numV, atoi(destId.c_str()));
    }
    topofile.close();
    return numV;
}

void createTopo(char *argv1, vvp & topo){
    cout << "createTopo..." << endl;
    ifstream topofile(argv1);
    string sourceId, destId, cost;
    // vector<vector<pair<int, int> > > topo(numV + 1);

    while(topofile >> sourceId >> destId >> cost){
        int u = atoi(sourceId.c_str());
        int v = atoi(destId.c_str());
        int w = atoi(cost.c_str());
        if (w != -999){
            topo[u].push_back(make_pair(v,w));
            topo[v].push_back(make_pair(u,w));
        }
    }
    topofile.close();
    /* check topo
    for(int i = 1; i <= numV ; i++){
        for(int j = 0; j < topo[i].size() ; j ++){
            cout << i << " " << topo[i][j].f << " " << topo[i][j].s << " ";
        }
        cout << endl;
    }
    */
}

void relax(vector<int> & d, vector<int> & previous, int u, int v, int w){
    if(d[u] != INT_MAX && d[u] + w < d[v]){
        d[v] = d[u] + w;
        previous[v] = u;
    }
}

void BellmanFord(int numV, vvp & topo,  vvi & dist,  vvi & prev){
    cout << "Bellman Ford Algorithm..." << endl;

    for(int i = 1; i <= numV; i++){ // find every short path of every node
        dist[i].clear();
        prev[i].clear();
        dist[i].resize(numV + 1, INT_MAX);
        prev[i].resize(numV + 1, -1);
        dist[i][i] = 0; // the start ting point has initial value 0
        for(int cnt = 0 ;  cnt < numV - 1 ; cnt++){ // maximum n-1 path in Bellman-Ford otherwise a cycle exists
            if(i == 6) cout << cnt << endl;
            for(int u = 1 ; u <= numV ; u++){
                for(int idx = 0 ; idx < topo[u].size(); idx++){
                    int v = topo[u][idx].f;
                    int w = topo[u][idx].s;
                    relax(dist[i], prev[i], u, v, w);
                }
            }
        }
    }
    // for(int i = 1; i <= numV; i++){
    //     for(int k = 1 ; k <= numV ; k++){
    //         cout << k << "  " << prev[i][k]<< endl;
    //     }
    //     cout << "------------------" << endl;
    // }
}

void printMessage(FILE* fp_output, string line, vvi & dist,  vvi & prev){
    istringstream iss(line);
    string word;
    string res;
    int sourceId;
    int destId;
    int i = 0;
    while (iss >> word) {
        if (i == 0) sourceId = stoi(word);
        if (i++ == 1) destId = stoi(word);
        if (i > 2) res += word + " ";
    }
    res.pop_back();
    if (dist[sourceId][destId] != INT_MAX){
        vector<int> path;
        
        int nexthop = destId; // don't need to use destId
        if (prev[sourceId][nexthop] == sourceId){
            path.push_back(destId);
        }
        while (prev[sourceId][nexthop] != sourceId){
            if (prev[sourceId][nexthop] == -1){
                nexthop = sourceId;
                path.push_back(nexthop);
                break;
            }
            nexthop = prev[sourceId][nexthop];
            path.push_back(nexthop);
        }
        path.push_back(sourceId);
        fprintf(fp_output, "from %d to %d cost %d hops ", sourceId, destId, dist[sourceId][destId]);
        for (int i = path.size() - 1; i >= 0; --i) {
            if (path[i] == destId) continue;
            fprintf(fp_output, "%d ", path[i]);
        }
        fprintf(fp_output, "message %s\n", res.c_str());
    }
    else{
        fprintf(fp_output, "from %d to %d cost infinite hops unreachable message %s\n", sourceId, destId, res.c_str());
    }
    fprintf(fp_output, "\n");
}

void printNodes(FILE* fp_output, int numVertices, vvi & dist,  vvi & prev) {
    for (int sourceId = 1; sourceId <= numVertices; ++sourceId) {
        for (int destId = 1; destId <= numVertices; ++destId) {
            if (dist[sourceId][destId] != INT_MAX) {
                // Find the path
                vector<int> path;
                int nexthop = destId; // don't need to use destId
                if (prev[sourceId][nexthop] == sourceId){
                    path.push_back(destId);
                }
                while (prev[sourceId][nexthop] != sourceId){
                    if (prev[sourceId][nexthop] == -1){
                        nexthop = sourceId;
                        path.push_back(nexthop);
                        break;
                    }
                    nexthop = prev[sourceId][nexthop];
                    path.push_back(nexthop);
                }
                fprintf(fp_output, "%d %d %d\n", destId, path.back(), dist[sourceId][destId]);
            }
        }
        fprintf(fp_output, "\n");
    }
}

bool updateWeight(int u, int v, int w, vvp &topo) {
    bool found = 0;
    for (int i = 0; i < topo[u].size(); i++) {
        if (topo[u][i].f == v) {
            topo[u][i].s = w;
            found = 1;
            break;
        }
    }
    for (int i = 0; i < topo[v].size(); i++) {
        if (topo[v][i].first == u) {
            topo[v][i].second = w;
            found = 1;
            break;
        }
    }
    return found;
}

void makeChanges(char* argv2, char* argv3, vvp &topo, int numV, FILE* fp_output, vvi dist, vvi prev){
    cout << argv3 << endl;
    cout << "nothing" << endl;
    ifstream changesFile(argv3);
    string Src, Dest, Weight;
    while(changesFile >> Src >> Dest >> Weight){
        int u = atoi(Src.c_str());
        int v = atoi(Dest.c_str());
        int w = atoi(Weight.c_str());
        if (w != -999){
            if (!updateWeight(u, v, w, topo)){
            topo[u].push_back(make_pair(v,w));
            topo[v].push_back(make_pair(u,w));
            }
        }
        else{
            for (int i = 0; i < topo[u].size(); i++) {
                if (topo[u][i].f == v) {
                    topo[u].erase(topo[u].begin() + i);
                    break;
                }
            }
            for (int i = 0; i < topo[v].size(); i++) {
                if (topo[v][i].f == u) {
                    topo[v].erase(topo[v].begin() + i);
                    break;
                }
            }
        }

        BellmanFord(numV, topo, dist, prev);
        printNodes(fp_output, numV, dist, prev);

        // Print the new messages
        ifstream msgFile(argv2);
        // msgFile.open(argv2);
        string line;
        while(getline(msgFile, line)){
            printMessage(fp_output, line, dist, prev);
        }
        msgFile.close();
    }
    changesFile.close();
}


int main(int argc, char **argv){
    if (argc != 4) {
        // ./distvec topofile messagefile changesfile
        cout << "Usage: ./distvec topofile messagefile changesfile\n" << endl;
        return -1;
    }
    //topofile
    int numV = countVertices(argv[1]);
    vvp topo(numV + 1);
    createTopo(argv[1], topo);
    vvi dist(numV + 1, vector<int>(numV + 1, INT_MAX));
    vvi prev(numV + 1, vector<int>(numV + 1, -1));
    // vector<int> dist(numV + 1, INT_MAX);
    // vector<int> prev(numV + 1, -1);

    BellmanFord(numV, topo, dist, prev);

    //messagefile
    FILE *fp_output;
    fp_output = fopen("output.txt", "w");

    // Initialize the routing table
    printNodes(fp_output, numV, dist, prev);

    // Read the message file and print
    ifstream messagefile(argv[2]);
    string line;
    while(getline(messagefile, line)){
        printMessage(fp_output, line, dist, prev);
    }
    messagefile.close();

    //changesfile
    makeChanges(argv[2], argv[3], topo, numV, fp_output, dist, prev);

    fclose(fp_output);
    return 0;
}

