#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using vl = vector<ll>;
using vvl = vector<vl>;
using vvvl = vector<vvl>;
using vi = vector<int>;
using vvi = vector<vi>;
using vvvi = vector<vvi>;
#define rep(i,a,b) for(int i = (a); i < (int)(b); i++)
#define all(v) v.begin(), v.end()

template <typename T>
T input(){
    T x;
    cin >> x;
    return x;
}

template <typename T>
inline bool chmax(T &a, const T& b){
    if(a < b){
        a = b;
        return true;
    }
    return false;
}

template <typename T>
inline bool chmin(T &a, const T& b){
    if(a > b){
        a = b;
        return true;
    }
    return false;
}

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

const long long INF = 1LL << 60;

int main(){

    std::ostream& fout = std::cout; // 標準出力用ストリーム

    int N, K, H, T, D;cin >> N >> K >> H >> T >> D;
    vector<vector<double>> own(K,vector<double>(3)); rep(i,0,K) cin >> own[i][0] >> own[i][1] >> own[i][2];
    vector<vector<double>> tar(H,vector<double>(3)); rep(i,0,H) cin >> tar[i][0] >> tar[i][1] >> tar[i][2];

    vector<vector<vector<int>>> canp(N,vector<vector<int>>(N,vector<int>(1,0)));
    vector<vector<double>> well(N*N,vector<double>(4,0));
    vector<vector<bool>> v(N, vector<bool>(N-1,0));
    vector<vector<bool>> h(N-1, vector<bool>(N,0));

    rep(i,0,N){
        rep(j,0,N-1){
            if(j == N/2-1){
                v[i][j] = 1;
            }
            fout << v[i][j];
            if(j != N-2){
                fout << " ";
            }
            else{
                fout << "\n";
            }
        }
    }
    rep(i,0,N-1){
        rep(j,0,N){
            fout << h[i][j];
            if(j != N-1){
                fout << " ";
            }
            else{
                fout << "\n";
            }
        }
    }

    double lc= 0, rc=0;
    double min = 100;
    rep(i,0,K){
        rep(j,i,K){
            double check = sqrt( (((own[i][0]+own[j][0])/2)-tar[0][0])*(((own[i][0]+own[j][0])/2)-tar[0][0])
                + (((own[i][1]+own[j][1])/2)-tar[0][1])*(((own[i][1]+own[j][1])/2)-tar[0][1])
                + (((own[i][2]+own[j][2])/2)-tar[0][2])*(((own[i][2]+own[j][2])/2)-tar[0][2]) );

            if (check < min){
                min = check;
                lc = i,rc = j;
            }
        }
    }
    double lc2= 0, rc2=0;
    min = 100;
    rep(i,0,K){
        rep(j,i,K){
            double check = sqrt( (((own[i][0]+own[j][0])/2)-0.5)*(((own[i][0]+own[j][0])/2)-0.5)
                + (((own[i][1]+own[j][1])/2)-0.5)*(((own[i][1]+own[j][1])/2)-0.5)
                + (((own[i][2]+own[j][2])/2)-0.5)*(((own[i][2]+own[j][2])/2)-0.5));

            if (check < min){
                min = check;
                lc2 = i,rc2 = j;
            }
        }
    }

    int sent = 0;

    int smin = 2, lmax = 1, rmax = 1;
    int size = 0, lsize =0, rsize =0;
    bool merged = false;
    while(sent < H){
        double k;
        int doing;
        int pi, pj;
        int pi2, pj2;
        if(6*(H-sent) > T && 3*(H-sent) > T){
            lmax = std::min((H-sent)/2+1,(N/2)*N);
            rmax = std::min((H-sent)/2+1,(N-N/2)*N);
            if(merged){
                if(size >= 1){
                    size --;
                    doing = 2;
                    pi = 0, pj = 0;
                    sent++;
                }
                else if(size <= 0){
                    size = 0;
                    doing = 4;
                    pi = 0, pj = N/2-1;
                    pi2 = 0, pj2 = N/2;
                    merged=0;
                }
                else{
                    size --;
                    doing = 3;
                    pi = 0; pj = 0;
                }

            }
            else if(lsize < lmax){
                doing = 1;
                pi = 0, pj = 0;
                k = lc2;
                lsize ++;
            }
            else if(rsize < rmax){
                doing = 1;
                pi = N-1, pj = N-1;
                k = rc2;
                rsize ++;
            }
            else{
                doing = 4;
                pi = 0, pj = N/2-1;
                pi2 = 0, pj2 = N/2;
                rsize = 0;
                lsize = 0;
                size = lmax + rmax;
                merged=1;
            }
        }
        else if(merged){
            if(size >= smin){
                size --;
                doing = 2;
                pi = 0, pj = 0;
                sent++;
                min = 100;
                if(sent != H){
                    rep(i,0,K){
                        rep(j,i,K){
                            double check = sqrt( (((own[i][0]+own[j][0])/2)-tar[sent][0])*(((own[i][0]+own[j][0])/2)-tar[sent][0])
                                + (((own[i][1]+own[j][1])/2)-tar[sent][1])*(((own[i][1]+own[j][1])/2)-tar[sent][1])
                                + (((own[i][2]+own[j][2])/2)-tar[sent][2])*(((own[i][2]+own[j][2])/2)-tar[sent][2]) );

                            if (check < min){
                                min = check;
                                lc = i,rc = j;
                            }
                        }
                    }
                }
            }
            else if(size <= 0){
                size = 0;
                doing = 4;
                pi = 0, pj = N/2-1;
                pi2 = 0, pj2 = N/2;
                merged=0;
            }
            else{
                size --;
                doing = 3;
                pi = 0; pj = 0;
            }

        }
        else if(lsize < lmax){
            doing = 1;
            pi = 0, pj = 0;
            k = lc;
            lsize ++;
        }
        else if(rsize < rmax){
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
            size = lmax + rmax;
            merged=1;
        }

        fout << doing << " ";
        if(doing == 1){
            T--;
            fout << pi << " " << pj << " " << k;
        }
        else if(doing == 2){
            T--;
            fout << pi << " " << pj;
        }
        else if(doing == 3){
            T--;
            fout << pi << " " << pj;
        }
        else if(doing == 4){
            T--;
            fout << pi << " " << pj << " " << pi2 << " " << pj2;
        }
        fout << "\n";
    }

}