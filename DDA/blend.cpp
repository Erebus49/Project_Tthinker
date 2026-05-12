// Fast DDA
#include <ilcplex/ilocplex.h>
#include "ListLinearHeap.h"
using namespace std;
ILOSTLBEGIN
int n, m;
int rm_idx;
ui *pstart, *edges;
ui *peel_sequence, *keys;
ui max_core, correpsond_size;
bool *rm;
double gama;
string file_path;
int cutoff_time;
float best_time;
int IP(int k, vector<int> &res)
{
    try
    {
        IloEnv env;
        IloModel model(env);
        IloBoolVarArray x(env, n);
        rm_idx=0;
        memset(rm,0,sizeof(bool)*n);
        while (rm_idx < n && keys[rm_idx] < k)
            rm[peel_sequence[rm_idx]] = true, rm_idx++;
        for (int i = 0; i < n; i++)
        {
            if (rm[i])
                continue;
            IloExpr neig_sum(env); // 用于保存所有 x[v] 的和
            for (int j = pstart[i]; j < pstart[i + 1]; j++)
                if (!rm[edges[j]])
                    neig_sum += x[edges[j]];
            model.add(k * x[i] <= neig_sum);
            neig_sum.end();
        }
        IloExpr sum(env);
        for (int i = 0; i < n; i++)
        {
            if (rm[i])
                continue;
            sum += x[i];
        }
        int ub = my_floor(k / gama) + 1;
        model.add(sum <= ub);
        IloObjective obj = IloMaximize(env, sum);
        model.add(obj);
        IloCplex cplex(model);
        cplex.setParam(IloCplex::Param::Advance, 0); 
        cplex.setParam(IloCplex::Param::Threads, 1);
        cplex.setOut(env.getNullStream());
        if (cplex.solve())
        {
            for (int i = 0; i < n; ++i)
            {
                if (rm[i])
                    continue;
                if (cplex.getValue(x[i]) < 0.5)
                    continue;
                res.push_back(i);
            }
        }
        else
        {
            exit(-1);
        }
        env.end();
        if (res.size())
            return res.size();
    }
    catch (...)
    {
        exit(-1);
    }
    return -1;
}
void check_graph_QC(vector<int> vset)
{
    int new_size = vset.size();
    int *st = new int[n];
    memset(st, 0, sizeof(int) * n);
    for (auto &v : vset)
        st[v] = 1;
    for (auto &u : vset)
    {
        int cnt = 0;
        for (int j = pstart[u]; j < pstart[u + 1]; j++)
        {
            if (st[edges[j]])
                cnt++;
        }
        if (cnt < gama * (new_size - 1))
        {
            cout << "res not satisfied !" << endl;
            cout << "deg: " << cnt << endl;
            exit(-1);
        }
    }
    delete[] st;
}
void read_from_file(string path)
{
    
    FILE *in = fopen(path.c_str(), "rb");
    if (in == nullptr)
    {
        //printf("Failed to open %s \n", file_path.c_str());
        exit(1);
    }
    ui size_int;
    fread(&size_int, sizeof(ui), 1, in);
    if (size_int != sizeof(ui))
    {
        //printf("sizeof int is different: graph_file(%u), machine(%u)\n", size_int, (int)sizeof(ui));
        exit(1);
    }
    fread(&n, sizeof(ui), 1, in);
    fread(&m, sizeof(ui), 1, in);
    //cout << "n=" << n << " m=" << m << endl;
    // cout << "File: " << get_file_name_without_suffix(file_path) << " n= " << n << " m= " << m / 2 << " k= " << paramK << endl;
    ui *degree = new ui[n];
    pstart = new ui[n + 1];
    edges = new ui[m];
    fread(degree, sizeof(ui), n, in);
    fread(edges, sizeof(ui), m, in);
    pstart[0] = 0;
    for (ui i = 1; i <= n; i++)
        pstart[i] = pstart[i - 1] + degree[i - 1];
    peel_sequence = new ui[n];
    keys = new ui[n];
    char *vis = new char[n];
    ListLinearHeap *heap = new ListLinearHeap(n, n - 1);
    memset(vis, 0, sizeof(char) * n);
    for (ui i = 0; i < n; i++)
        peel_sequence[i] = i;

    max_core = 0;
    heap->init(n, n - 1, peel_sequence, degree);
    for (ui i = 0; i < n; i++)
    {
        ui u, key;
        heap->pop_min(u, key);
        keys[i] = key;
        if (max_core < key)
        {
            max_core = key;
            correpsond_size = n - i;
        }
        peel_sequence[i] = u;
        vis[u] = 1;
        for (ui j = pstart[u]; j < pstart[u + 1]; j++)
            if (vis[edges[j]] == 0)
            {
                heap->decrement(edges[j], 1);
            }
    }
    delete heap;
    delete[] vis;
    delete[] degree;
}
size_t getMemoryUsage() {
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("VmHWM") != std::string::npos) {  // 最大物理内存占用
            return std::stoul(line.substr(line.find_last_of('\t') + 1)); // KB
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    //printf("\n-----------------------------------------------------------------------------------------\n");
    file_path = string(argv[1]);
    //cout << file_path << endl;
    gama = atof(argv[2]);
    cutoff_time = atoi(argv[3]);
    long long start = clock();
    read_from_file(file_path);
    rm = new bool[n];
    memset(rm, 0, sizeof(bool) * n);
    int currentMax = 0;
    int bestMax = 0;
    int k = max_core + 1;
    vector<int> res;
    //cout << "Start Core:" << k << endl;
    // if(currentMax<my_floor((correpsond_size-1)*gama))
    while (1)
    {
        k = k - 1;
        res.clear();
        
        //currentMax = max(IP(k, res), currentMax);
        currentMax = IP(k, res);
        if (currentMax > bestMax) {
            bestMax = currentMax;
            best_time = double(clock() - start) / CLOCKS_PER_SEC;
        }
        //cout << "Checking K:" << k << " currentMax:" << currentMax << " Find size:" << res.size() << endl;
        if (currentMax >= my_floor(k / gama) + 1)
            break;
        if ((double(clock() - start) / CLOCKS_PER_SEC) >= cutoff_time)
            break;
    }
    cout << file_path << " res: " << bestMax << " find time: " << best_time; // << endl;
    cout << " Running Time: " << double(clock() - start) / CLOCKS_PER_SEC; // << endl;
    cout << " cur k:" << k << endl;
    //cout<<getMemoryUsage()/1024.0/1024.0<<"MB"<<endl;
    check_graph_QC(res);
    delete[] peel_sequence;
    delete[] pstart;
    delete[] edges;
    delete[] rm;
    return 0;
}
