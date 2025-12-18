#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using vl = vector<ll>;                                  //long long型の一次元
using vvl = vector<vl>;                                 //long long型の二次元配列
using vvvl = vector<vvl>;                               //long long型の三次元配列
using vi = vector<int>;                                 //int型の一次元
using vvi = vector<vi>;                                 //int型の二次元配列
using vvvi = vector<vvi>;                               //int型の三次元配列
#define rep(i,a,b) for(int i = (a); i < (int)(b); i++)  //for文の短縮
#define all(v) v.begin(), v.end()                       //all(v)でvの始まりと終わりのイテレーター

//入力を受け取る
template <typename T> 
T input(){
    T x;
    cin >> x;
    return x;
}

//a,bのうち最大のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmax(T &a, const T& b){
    if(a < b){
        a = b;
        return true;
    }
    return false;
}

//a,bのうち最小のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmin(T &a, const T& b){
    if(a > b){
        a = b;
        return true;
    }
    return false;
}

//素数判定
bool is_prime(long long n) {
    if (n <= 1) {
        return false;
    }
    for (long long i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

// 無限大の値
const long long INF = 1LL << 60;

int main(){

    int N, K, H, R, D;cin >> N >> K >> H >> R >> D;
    vector<vector<double>> own(K,vector<double>(3)); rep(i,0,K) cin >> own[i][0] >> own[i][1] >> own[i][2];
    vector<vector<double>> tar(H,vector<double>(3)); rep(i,0,H) cin >> tar[i][0] >> tar[i][1] >> tar[i][2];

    vector<vector<vector<int>>> canp(N,vector<vector<int>>(N,vector<int>(1,0)));
    vector<vector<double>> well(N*N,vector<double>(4,0));
    vector<vector<bool>> v(N, vector<bool>(N-1,0));   //マス(i,j)とマス(i,j+1)の間の縦の仕切りの状態
    vector<vector<bool>> h(N-1, vector<bool>(N,0));   //マス(i,j)とマス(i+1,j)の間の横の仕切りの状態

    rep(i,0,N){
        rep(j,0,N-1){
            if(j == N/2-1){
                v[i][j] = 1;
            }
            cout << v[i][j];
            if(j != N-2){
                cout << " ";
            }
            else{
                cout << "\n";
            }
        }
    }
    rep(i,0,N-1){
        rep(j,0,N){
            cout << h[i][j];
            if(j != N-1){
                cout << " ";
            }
            else{
                cout << "\n";
            }
        }
    }

    /*vector<vector<int>> seen(N, vector<int>(N,0));
    queue<pair<int, int>> q;
    vector<int> dx = {0,1,0,-1};
    vector<int> dy = {1,0,-1,0};
    int w = 0;
    q.push({0,0});
    while(!q.empty()){
        auto[x, y] = q.front();
        q.pop();
        if(seen[x][y]) continue;
        seen[x][y] = 1;
        canp[x][y][0] = w;
        well[w][3] ++;
        for(int i=0; i<4; i++){
            int nx = x + dx[i];
            int ny = y + dy[i];
            if(nx < 0 || nx >= N){
                continue;
            }
            if(ny < 0 || ny >= N){
                continue;
            }            
            if(seen[nx][ny]) continue;
            if(i == 0 || i == 2){
                if(v[(nx+x)/2][(ny+y)/2]){
                    continue;
                }
            }
            if(i == 1 || i == 3){
                if(h[(nx+x)/2][(ny+y)/2]){
                    continue;
                }
            }
            q.push({nx, ny});
        }
        if(q.empty()){
            bool fend = false;
            rep(i,0,N){
                if(fend){
                    break;
                }
                rep(j,0,N){
                    if(!seen[i][j]){
                        q.push({i,j});
                        fend = true;
                        w++;
                        break;
                    }
                }
            }
        }            
    }*/

    double lc= 0, rc=0;
    vector<double> col(3,0);
    double min = 100;
    rep(i,0,K){
        rep(j,i,K){
            double check = sqrt( (((own[i][0]+own[j][0])/2)-0.5)*(((own[i][0]+own[j][0])/2)-0.5) 
                + (((own[i][1]+own[j][1])/2)-0.5)*(((own[i][1]+own[j][1])/2)-0.5)
                + (((own[i][2]+own[j][2])/2)-0.5)*(((own[i][2]+own[j][2])/2)-0.5));

            if (check < min){
                min = check;
                lc = i,rc = j;
            }
        }
    }

    int sent = 0;

    int size = 0, lsize =0, rsize =0;
    bool merged = false, next = false; 
    while(sent < H){    
        double k;
        int doing;
        int pi, pj;
        int pi2, pj2;
        if(next){
            next = false;
            doing = 4;
            pi = 0, pj = N/2-1;
            pi2 = 0, pj2 = N/2;
            merged=0;
        }
        else if(merged){
            if(size >= 1){
                size --;
                doing = 2;
                pi = 0, pj = 0;
                sent++;
            }
            else{
                size = 0;
                doing = 3;
                pi = 0; pj = 0;
                next = 1;
            }

        }
        else if(lsize < (N/2)*N){
            doing = 1;
            pi = 0, pj = 0;
            k = lc;
            lsize ++;
        }
        else if(rsize < (N-N/2)*N){
            doing = 1;
            pi = N-1, pj = N-1;
            k = rc;
            rsize ++;
        }
        else{
            doing = 4;
            pi = 0, pj = N/2-1;
            pi2 = 0, pj2 = N/2;
            rsize = 0;
            lsize = 0;
            size = N*N;
            merged=1;
        }

        cout << doing << " ";
        if(doing == 1){
            cout << pi << " " << pj << " " << k;
        }
        else if(doing == 2){
            cout << pi << " " << pj;
        }
        else if(doing == 3){
            cout << pi << " " << pj;
        }
        else if(doing == 4){
            cout << pi << " " << pj << " " << pi2 << " " << pj2;
        }
        cout << "\n";
    }

}