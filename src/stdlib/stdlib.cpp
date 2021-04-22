#include <cstdio>

extern "C" int64_t giveYouUp0();

extern "C" void print(int64_t in){
	printf("%lld\n", in);
}

extern "C" int64_t in(){
    int64_t res = 0;
	scanf("%lli", &res);
	return res;
}

int main(){
    return giveYouUp0();
}
