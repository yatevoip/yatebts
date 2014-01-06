#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <a53.h>
using namespace std;

void printer(int which, const char *label, u8 *vector, int lth)
{
	cout << " block" << which << " " << label << " = ";
	int i;
	for (i = 0; i < lth; i++) {
		cout << hex << std::setw(2) << setfill('0') << (int)vector[i] << dec;
	}
}

void check(int which, u8 *exp, u8 *got)
{
	printer(which, "exp", exp, 15);
	printer(which, "got", got, 15);
	int err = 0;
	int i;
	for (i = 0; i < 15; i++) {
		if (exp[i] == got[i]) continue;
		err = 1;
		break;
	}
	if (err) {
		cout << " ERROR" << endl;
	} else {
		cout << " ok" << endl;
	}
}

int unhex(const char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	cout << "oops" << endl;
	exit(1);
}

int seti(const char *src)
{
	int r = 0;
	for ( ; *src != 0; src++) {
		r = (r << 4) + unhex(*src);
	}
	return r;
}

void setu8(u8 *dst, const char *src)
{
	for ( ; *src != 0; src += 2) {
		*dst++ = (unhex(src[0]) << 4) | (unhex(src[1]));
	}
}

int main()
{
	int i, keylth;
	unsigned int count;
	u8 key[16], exp1[15], exp2[15], got1[15], got2[15];
	const char *data[52] = {
		"2BD6459F82C5BC00", "24F20F", "889EEAAF9ED1BA1ABBD8436232E440", "5CA3406AA244CF69CF047AADA2DF40",
		"952C49104881FF48", "061527", "AB7DB38A573A325DAA76E4CB800A40", "4C4B594FEA9D00FE8978B7B7BC1080",
		"EFA8B2229E720C2A", "33FD3F", "0E4015755A336469C3DD8680E30340", "6F10669E2B4E18B042431A28E47F80",
		"3451F23A43BD2C87", "0E418C", "75F7C4C51560905DFBA05E46FB54C0", "192C95353CDF979E054186DF15BF00",
		"CAA2639BE82435CF", "2FF229", "301437E4D4D6565D4904C631606EC0", "F0A3B8795E264D3E1A82F684353DC0",
		"7AE67E87400B9FA6", "2F24E5", "F794290FEF643D2EA348A7796A2100", "CB6FA6C6B8A705AF9FEFE975818500",
		"58AF69935540698B", "05446B", "749CA4E6B691E5A598C461D5FE4740", "31C9E444CD04677ADAA8A082ADBC40",
		"017F81E5F236FE62", "156B26", "2A6976761E60CC4E8F9F52160276C0", "A544D8475F2C78C35614128F1179C0",
		"1ACA8B448B767B39", "0BC3B5", "A4F70DC5A2C9707F5FA1C60EB10640", "7780B597B328C1400B5C74823E8500",
		// "5ACB1D644C0D512041A5", "1D5157", "8EFAEC49C355CCD995C2BF649FD480", "F3A2910CAEDF587E976171AAF33B80",
		// "9315819243A043BEBE6E", "2E196F", "AA08DB46DD3DED78A612085C529D00", "0250463DA0E3886F9BC2E3BB0D73C0",
		// "3D43C388C9581E337FF1F97EB5C1F85E", "35D2CF", "A2FE3034B6B22CC4E33C7090BEC340", "170D7497432FF897B91BE8AECBA880",
		// "A4496A64DF4F399F3B4506814A3E07A1", "212777", "89CDEE360DF9110281BCF57755A040", "33822C0C779598C9CBFC49183AF7C0",
	};
	for (i = 32; i>=0; i-=4) {
		cout << "test set " << (1+i/4) << endl;
		setu8(key, data[i]);
		keylth = strlen(data[i])*4;
		count = seti(data[i+1]);
		A53_GSM(key, keylth, count, got1, got2);
		setu8(exp1, data[i+2]);
		setu8(exp2, data[i+3]);
		check(1, exp1, got1);
		check(2, exp2, got2);
	}
	cout << "time test" << endl;
	int n = 10000;
	float t = clock();
	for (i = 0; i < n; i++) {
		A53_GSM(key, keylth, count, got1, got2);
	}
	t = (clock() - t) / (CLOCKS_PER_SEC * (float)n);
	cout << "GSM takes " << t << " seconds per iteration" << endl;
	exit(0);
}
