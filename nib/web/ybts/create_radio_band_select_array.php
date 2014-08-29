<?php
/**
 * create_radio_band_select_array.php
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Copyright (C) 2014 Null Team
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

function prepare_gsm_field_radio_c0()
{
$valid_values =  "INVALID|GSM850
128| #128 : 869.2 MHz downlink / 824.2 MHz uplink
129| #129 : 869.4 MHz downlink / 824.4 MHz uplink
130| #130 : 869.6 MHz downlink / 824.6 MHz uplink
131| #131 : 869.8 MHz downlink / 824.8 MHz uplink
132| #132 : 870 MHz downlink / 825 MHz uplink
133| #133 : 870.2 MHz downlink / 825.2 MHz uplink
134| #134 : 870.4 MHz downlink / 825.4 MHz uplink
135| #135 : 870.6 MHz downlink / 825.6 MHz uplink
136| #136 : 870.8 MHz downlink / 825.8 MHz uplink
137| #137 : 871 MHz downlink / 826 MHz uplink
138| #138 : 871.2 MHz downlink / 826.2 MHz uplink
139| #139 : 871.4 MHz downlink / 826.4 MHz uplink
140| #140 : 871.6 MHz downlink / 826.6 MHz uplink
141| #141 : 871.8 MHz downlink / 826.8 MHz uplink
142| #142 : 872 MHz downlink / 827 MHz uplink
143| #143 : 872.2 MHz downlink / 827.2 MHz uplink
144| #144 : 872.4 MHz downlink / 827.4 MHz uplink
145| #145 : 872.6 MHz downlink / 827.6 MHz uplink
146| #146 : 872.8 MHz downlink / 827.8 MHz uplink
147| #147 : 873 MHz downlink / 828 MHz uplink
148| #148 : 873.2 MHz downlink / 828.2 MHz uplink
149| #149 : 873.4 MHz downlink / 828.4 MHz uplink
150| #150 : 873.6 MHz downlink / 828.6 MHz uplink
151| #151 : 873.8 MHz downlink / 828.8 MHz uplink
152| #152 : 874 MHz downlink / 829 MHz uplink
153| #153 : 874.2 MHz downlink / 829.2 MHz uplink
154| #154 : 874.4 MHz downlink / 829.4 MHz uplink
155| #155 : 874.6 MHz downlink / 829.6 MHz uplink
156| #156 : 874.8 MHz downlink / 829.8 MHz uplink
157| #157 : 875 MHz downlink / 830 MHz uplink
158| #158 : 875.2 MHz downlink / 830.2 MHz uplink
159| #159 : 875.4 MHz downlink / 830.4 MHz uplink
160| #160 : 875.6 MHz downlink / 830.6 MHz uplink
161| #161 : 875.8 MHz downlink / 830.8 MHz uplink
162| #162 : 876 MHz downlink / 831 MHz uplink
163| #163 : 876.2 MHz downlink / 831.2 MHz uplink
164| #164 : 876.4 MHz downlink / 831.4 MHz uplink
165| #165 : 876.6 MHz downlink / 831.6 MHz uplink
166| #166 : 876.8 MHz downlink / 831.8 MHz uplink
167| #167 : 877 MHz downlink / 832 MHz uplink
168| #168 : 877.201 MHz downlink / 832.201 MHz uplink
169| #169 : 877.401 MHz downlink / 832.401 MHz uplink
170| #170 : 877.601 MHz downlink / 832.601 MHz uplink
171| #171 : 877.801 MHz downlink / 832.801 MHz uplink
172| #172 : 878.001 MHz downlink / 833.001 MHz uplink
173| #173 : 878.201 MHz downlink / 833.201 MHz uplink
174| #174 : 878.401 MHz downlink / 833.401 MHz uplink
175| #175 : 878.601 MHz downlink / 833.601 MHz uplink
176| #176 : 878.801 MHz downlink / 833.801 MHz uplink
177| #177 : 879.001 MHz downlink / 834.001 MHz uplink
178| #178 : 879.201 MHz downlink / 834.201 MHz uplink
179| #179 : 879.401 MHz downlink / 834.401 MHz uplink
180| #180 : 879.601 MHz downlink / 834.601 MHz uplink
181| #181 : 879.801 MHz downlink / 834.801 MHz uplink
182| #182 : 880.001 MHz downlink / 835.001 MHz uplink
183| #183 : 880.201 MHz downlink / 835.201 MHz uplink
184| #184 : 880.401 MHz downlink / 835.401 MHz uplink
185| #185 : 880.601 MHz downlink / 835.601 MHz uplink
186| #186 : 880.801 MHz downlink / 835.801 MHz uplink
187| #187 : 881.001 MHz downlink / 836.001 MHz uplink
188| #188 : 881.201 MHz downlink / 836.201 MHz uplink
189| #189 : 881.401 MHz downlink / 836.401 MHz uplink
190| #190 : 881.601 MHz downlink / 836.601 MHz uplink
191| #191 : 881.801 MHz downlink / 836.801 MHz uplink
192| #192 : 882.001 MHz downlink / 837.001 MHz uplink
193| #193 : 882.201 MHz downlink / 837.201 MHz uplink
194| #194 : 882.401 MHz downlink / 837.401 MHz uplink
195| #195 : 882.601 MHz downlink / 837.601 MHz uplink
196| #196 : 882.801 MHz downlink / 837.801 MHz uplink
197| #197 : 883.001 MHz downlink / 838.001 MHz uplink
198| #198 : 883.201 MHz downlink / 838.201 MHz uplink
199| #199 : 883.401 MHz downlink / 838.401 MHz uplink
200| #200 : 883.601 MHz downlink / 838.601 MHz uplink
201| #201 : 883.801 MHz downlink / 838.801 MHz uplink
202| #202 : 884.001 MHz downlink / 839.001 MHz uplink
203| #203 : 884.201 MHz downlink / 839.201 MHz uplink
204| #204 : 884.401 MHz downlink / 839.401 MHz uplink
205| #205 : 884.601 MHz downlink / 839.601 MHz uplink
206| #206 : 884.801 MHz downlink / 839.801 MHz uplink
207| #207 : 885.001 MHz downlink / 840.001 MHz uplink
208| #208 : 885.201 MHz downlink / 840.201 MHz uplink
209| #209 : 885.401 MHz downlink / 840.401 MHz uplink
210| #210 : 885.601 MHz downlink / 840.601 MHz uplink
211| #211 : 885.801 MHz downlink / 840.801 MHz uplink
212| #212 : 886.001 MHz downlink / 841.001 MHz uplink
213| #213 : 886.201 MHz downlink / 841.201 MHz uplink
214| #214 : 886.401 MHz downlink / 841.401 MHz uplink
215| #215 : 886.601 MHz downlink / 841.601 MHz uplink
216| #216 : 886.801 MHz downlink / 841.801 MHz uplink
217| #217 : 887.001 MHz downlink / 842.001 MHz uplink
218| #218 : 887.201 MHz downlink / 842.201 MHz uplink
219| #219 : 887.401 MHz downlink / 842.401 MHz uplink
220| #220 : 887.601 MHz downlink / 842.601 MHz uplink
221| #221 : 887.801 MHz downlink / 842.801 MHz uplink
222| #222 : 888.001 MHz downlink / 843.001 MHz uplink
223| #223 : 888.201 MHz downlink / 843.201 MHz uplink
224| #224 : 888.401 MHz downlink / 843.401 MHz uplink
225| #225 : 888.601 MHz downlink / 843.601 MHz uplink
226| #226 : 888.801 MHz downlink / 843.801 MHz uplink
227| #227 : 889.001 MHz downlink / 844.001 MHz uplink
228| #228 : 889.201 MHz downlink / 844.201 MHz uplink
229| #229 : 889.401 MHz downlink / 844.401 MHz uplink
230| #230 : 889.601 MHz downlink / 844.601 MHz uplink
231| #231 : 889.801 MHz downlink / 844.801 MHz uplink
232| #232 : 890.001 MHz downlink / 845.001 MHz uplink
233| #233 : 890.201 MHz downlink / 845.201 MHz uplink
234| #234 : 890.401 MHz downlink / 845.401 MHz uplink
235| #235 : 890.601 MHz downlink / 845.601 MHz uplink
236| #236 : 890.801 MHz downlink / 845.801 MHz uplink
237| #237 : 891.001 MHz downlink / 846.001 MHz uplink
238| #238 : 891.201 MHz downlink / 846.201 MHz uplink
239| #239 : 891.401 MHz downlink / 846.401 MHz uplink
240| #240 : 891.601 MHz downlink / 846.601 MHz uplink
241| #241 : 891.801 MHz downlink / 846.801 MHz uplink
242| #242 : 892.001 MHz downlink / 847.001 MHz uplink
243| #243 : 892.201 MHz downlink / 847.201 MHz uplink
244| #244 : 892.401 MHz downlink / 847.401 MHz uplink
245| #245 : 892.601 MHz downlink / 847.601 MHz uplink
246| #246 : 892.801 MHz downlink / 847.801 MHz uplink
247| #247 : 893.001 MHz downlink / 848.001 MHz uplink
248| #248 : 893.201 MHz downlink / 848.201 MHz uplink
249| #249 : 893.401 MHz downlink / 848.401 MHz uplink
250| #250 : 893.602 MHz downlink / 848.602 MHz uplink
251| #251 : 893.802 MHz downlink / 848.802 MHz uplink
INVALID|EGSM900
0| #0: 935 MHz downlink / 890 MHz uplink
1| #1: 935.2 MHz downlink / 890.2 MHz uplink
2| #2: 935.4 MHz downlink / 890.4 MHz uplink
3| #3: 935.6 MHz downlink / 890.6 MHz uplink
4| #4: 935.8 MHz downlink / 890.8 MHz uplink
5| #5: 936 MHz downlink / 891 MHz uplink
6| #6: 936.2 MHz downlink / 891.2 MHz uplink
7| #7: 936.4 MHz downlink / 891.4 MHz uplink
8| #8: 936.6 MHz downlink / 891.6 MHz uplink
9| #9: 936.8 MHz downlink / 891.8 MHz uplink
10| #10: 937 MHz downlink / 892 MHz uplink
11| #11: 937.2 MHz downlink / 892.2 MHz uplink
12| #12: 937.4 MHz downlink / 892.4 MHz uplink
13| #13: 937.6 MHz downlink / 892.6 MHz uplink
14| #14: 937.8 MHz downlink / 892.8 MHz uplink
15| #15: 938 MHz downlink / 893 MHz uplink
16| #16: 938.2 MHz downlink / 893.2 MHz uplink
17| #17: 938.4 MHz downlink / 893.4 MHz uplink
18| #18: 938.6 MHz downlink / 893.6 MHz uplink
19| #19: 938.8 MHz downlink / 893.8 MHz uplink
20| #20: 939 MHz downlink / 894 MHz uplink
21| #21: 939.2 MHz downlink / 894.2 MHz uplink
22| #22: 939.4 MHz downlink / 894.4 MHz uplink
23| #23: 939.6 MHz downlink / 894.6 MHz uplink
24| #24: 939.8 MHz downlink / 894.8 MHz uplink
25| #25: 940 MHz downlink / 895 MHz uplink
26| #26: 940.2 MHz downlink / 895.2 MHz uplink
27| #27: 940.4 MHz downlink / 895.4 MHz uplink
28| #28: 940.6 MHz downlink / 895.6 MHz uplink
29| #29: 940.8 MHz downlink / 895.8 MHz uplink
30| #30: 941 MHz downlink / 896 MHz uplink
31| #31: 941.2 MHz downlink / 896.2 MHz uplink
32| #32: 941.4 MHz downlink / 896.4 MHz uplink
33| #33: 941.6 MHz downlink / 896.6 MHz uplink
34| #34: 941.8 MHz downlink / 896.8 MHz uplink
35| #35: 942 MHz downlink / 897 MHz uplink
36| #36: 942.2 MHz downlink / 897.2 MHz uplink
37| #37: 942.4 MHz downlink / 897.4 MHz uplink
38| #38: 942.6 MHz downlink / 897.6 MHz uplink
39| #39: 942.8 MHz downlink / 897.8 MHz uplink
40| #40: 943 MHz downlink / 898 MHz uplink
41| #41: 943.2 MHz downlink / 898.2 MHz uplink
42| #42: 943.4 MHz downlink / 898.4 MHz uplink
43| #43: 943.6 MHz downlink / 898.6 MHz uplink
44| #44: 943.8 MHz downlink / 898.8 MHz uplink
45| #45: 944 MHz downlink / 899 MHz uplink
46| #46: 944.2 MHz downlink / 899.2 MHz uplink
47| #47: 944.4 MHz downlink / 899.4 MHz uplink
48| #48: 944.6 MHz downlink / 899.6 MHz uplink
49| #49: 944.8 MHz downlink / 899.8 MHz uplink
50| #50: 945 MHz downlink / 900 MHz uplink
51| #51: 945.2 MHz downlink / 900.2 MHz uplink
52| #52: 945.4 MHz downlink / 900.4 MHz uplink
53| #53: 945.6 MHz downlink / 900.6 MHz uplink
54| #54: 945.8 MHz downlink / 900.8 MHz uplink
55| #55: 946 MHz downlink / 901 MHz uplink
56| #56: 946.2 MHz downlink / 901.2 MHz uplink
57| #57: 946.4 MHz downlink / 901.4 MHz uplink
58| #58: 946.6 MHz downlink / 901.6 MHz uplink
59| #59: 946.8 MHz downlink / 901.8 MHz uplink
60| #60: 947 MHz downlink / 902 MHz uplink
61| #61: 947.2 MHz downlink / 902.2 MHz uplink
62| #62: 947.4 MHz downlink / 902.4 MHz uplink
63| #63: 947.6 MHz downlink / 902.6 MHz uplink
64| #64: 947.8 MHz downlink / 902.8 MHz uplink
65| #65: 948 MHz downlink / 903 MHz uplink
66| #66: 948.2 MHz downlink / 903.2 MHz uplink
67| #67: 948.4 MHz downlink / 903.4 MHz uplink
68| #68: 948.6 MHz downlink / 903.6 MHz uplink
69| #69: 948.8 MHz downlink / 903.8 MHz uplink
70| #70: 949 MHz downlink / 904 MHz uplink
71| #71: 949.2 MHz downlink / 904.2 MHz uplink
72| #72: 949.4 MHz downlink / 904.4 MHz uplink
73| #73: 949.6 MHz downlink / 904.6 MHz uplink
74| #74: 949.8 MHz downlink / 904.8 MHz uplink
75| #75: 950 MHz downlink / 905 MHz uplink
76| #76: 950.2 MHz downlink / 905.2 MHz uplink
77| #77: 950.4 MHz downlink / 905.4 MHz uplink
78| #78: 950.6 MHz downlink / 905.6 MHz uplink
79| #79: 950.8 MHz downlink / 905.8 MHz uplink
80| #80: 951 MHz downlink / 906 MHz uplink
81| #81: 951.2 MHz downlink / 906.2 MHz uplink
82| #82: 951.4 MHz downlink / 906.4 MHz uplink
83| #83: 951.6 MHz downlink / 906.6 MHz uplink
84| #84: 951.8 MHz downlink / 906.8 MHz uplink
85| #85: 952 MHz downlink / 907 MHz uplink
86| #86: 952.2 MHz downlink / 907.2 MHz uplink
87| #87: 952.4 MHz downlink / 907.4 MHz uplink
88| #88: 952.6 MHz downlink / 907.6 MHz uplink
89| #89: 952.8 MHz downlink / 907.8 MHz uplink
90| #90: 953 MHz downlink / 908 MHz uplink
91| #91: 953.2 MHz downlink / 908.2 MHz uplink
92| #92: 953.4 MHz downlink / 908.4 MHz uplink
93| #93: 953.6 MHz downlink / 908.6 MHz uplink
94| #94: 953.8 MHz downlink / 908.8 MHz uplink
95| #95: 954 MHz downlink / 909 MHz uplink
96| #96: 954.2 MHz downlink / 909.2 MHz uplink
97| #97: 954.4 MHz downlink / 909.4 MHz uplink
98| #98: 954.6 MHz downlink / 909.6 MHz uplink
99| #99: 954.8 MHz downlink / 909.8 MHz uplink
100| #100: 955 MHz downlink / 910 MHz uplink
101| #101: 955.2 MHz downlink / 910.2 MHz uplink
102| #102: 955.4 MHz downlink / 910.4 MHz uplink
103| #103: 955.6 MHz downlink / 910.6 MHz uplink
104| #104: 955.8 MHz downlink / 910.8 MHz uplink
105| #105: 956 MHz downlink / 911 MHz uplink
106| #106: 956.2 MHz downlink / 911.2 MHz uplink
107| #107: 956.4 MHz downlink / 911.4 MHz uplink
108| #108: 956.6 MHz downlink / 911.6 MHz uplink
109| #109: 956.8 MHz downlink / 911.8 MHz uplink
110| #110: 957 MHz downlink / 912 MHz uplink
111| #111: 957.2 MHz downlink / 912.2 MHz uplink
112| #112: 957.4 MHz downlink / 912.4 MHz uplink
113| #113: 957.6 MHz downlink / 912.6 MHz uplink
114| #114: 957.8 MHz downlink / 912.8 MHz uplink
115| #115: 958 MHz downlink / 913 MHz uplink
116| #116: 958.2 MHz downlink / 913.2 MHz uplink
117| #117: 958.4 MHz downlink / 913.4 MHz uplink
118| #118: 958.6 MHz downlink / 913.6 MHz uplink
119| #119: 958.8 MHz downlink / 913.8 MHz uplink
120| #120: 959 MHz downlink / 914 MHz uplink
121| #121: 959.2 MHz downlink / 914.2 MHz uplink
122| #122: 959.4 MHz downlink / 914.4 MHz uplink
123| #123: 959.6 MHz downlink / 914.6 MHz uplink
124| #124: 959.8 MHz downlink / 914.8 MHz uplink
975| #975: 925.2 MHz downlink / 880.2 MHz uplink
976| #976: 925.4 MHz downlink / 880.4 MHz uplink
977| #977: 925.6 MHz downlink / 880.6 MHz uplink
978| #978: 925.8 MHz downlink / 880.8 MHz uplink
979| #979: 926 MHz downlink / 881 MHz uplink
980| #980: 926.2 MHz downlink / 881.2 MHz uplink
981| #981: 926.4 MHz downlink / 881.4 MHz uplink
982| #982: 926.6 MHz downlink / 881.6 MHz uplink
983| #983: 926.8 MHz downlink / 881.8 MHz uplink
984| #984: 927 MHz downlink / 882 MHz uplink
985| #985: 927.2 MHz downlink / 882.2 MHz uplink
986| #986: 927.4 MHz downlink / 882.4 MHz uplink
987| #987: 927.6 MHz downlink / 882.6 MHz uplink
988| #988: 927.8 MHz downlink / 882.8 MHz uplink
989| #989: 928 MHz downlink / 883 MHz uplink
990| #990: 928.2 MHz downlink / 883.2 MHz uplink
991| #991: 928.4 MHz downlink / 883.4 MHz uplink
992| #992: 928.6 MHz downlink / 883.6 MHz uplink
993| #993: 928.8 MHz downlink / 883.8 MHz uplink
994| #994: 929 MHz downlink / 884 MHz uplink
995| #995: 929.2 MHz downlink / 884.2 MHz uplink
996| #996: 929.4 MHz downlink / 884.4 MHz uplink
997| #997: 929.6 MHz downlink / 884.6 MHz uplink
998| #998: 929.8 MHz downlink / 884.8 MHz uplink
999| #999: 930 MHz downlink / 885 MHz uplink
1000| #1000: 930.2 MHz downlink / 885.2 MHz uplink
1001| #1001: 930.4 MHz downlink / 885.4 MHz uplink
1002| #1002: 930.6 MHz downlink / 885.6 MHz uplink
1003| #1003: 930.8 MHz downlink / 885.8 MHz uplink
1004| #1004: 931 MHz downlink / 886 MHz uplink
1005| #1005: 931.2 MHz downlink / 886.2 MHz uplink
1006| #1006: 931.4 MHz downlink / 886.4 MHz uplink
1007| #1007: 931.6 MHz downlink / 886.6 MHz uplink
1008| #1008: 931.8 MHz downlink / 886.8 MHz uplink
1009| #1009: 932 MHz downlink / 887 MHz uplink
1010| #1010: 932.2 MHz downlink / 887.2 MHz uplink
1011| #1011: 932.4 MHz downlink / 887.4 MHz uplink
1012| #1012: 932.6 MHz downlink / 887.6 MHz uplink
1013| #1013: 932.8 MHz downlink / 887.8 MHz uplink
1014| #1014: 933 MHz downlink / 888 MHz uplink
1015| #1015: 933.2 MHz downlink / 888.2 MHz uplink
1016| #1016: 933.4 MHz downlink / 888.4 MHz uplink
1017| #1017: 933.6 MHz downlink / 888.6 MHz uplink
1018| #1018: 933.8 MHz downlink / 888.8 MHz uplink
1019| #1019: 934 MHz downlink / 889 MHz uplink
1020| #1020: 934.2 MHz downlink / 889.2 MHz uplink
1021| #1021: 934.4 MHz downlink / 889.4 MHz uplink
1022| #1022: 934.6 MHz downlink / 889.6 MHz uplink
1023| #1023: 934.8 MHz downlink / 889.8 MHz uplink
INVALID|DCS1800
512| #512 : 1805.2 MHz downlink / 1710.2 MHz uplink
513| #513 : 1805.4 MHz downlink / 1710.4 MHz uplink
514| #514 : 1805.6 MHz downlink / 1710.6 MHz uplink
515| #515 : 1805.8 MHz downlink / 1710.8 MHz uplink
516| #516 : 1806 MHz downlink / 1711 MHz uplink
517| #517 : 1806.2 MHz downlink / 1711.2 MHz uplink
518| #518 : 1806.4 MHz downlink / 1711.4 MHz uplink
519| #519 : 1806.6 MHz downlink / 1711.6 MHz uplink
520| #520 : 1806.8 MHz downlink / 1711.8 MHz uplink
521| #521 : 1807 MHz downlink / 1712 MHz uplink
522| #522 : 1807.2 MHz downlink / 1712.2 MHz uplink
523| #523 : 1807.4 MHz downlink / 1712.4 MHz uplink
524| #524 : 1807.6 MHz downlink / 1712.6 MHz uplink
525| #525 : 1807.8 MHz downlink / 1712.8 MHz uplink
526| #526 : 1808 MHz downlink / 1713 MHz uplink
527| #527 : 1808.2 MHz downlink / 1713.2 MHz uplink
528| #528 : 1808.4 MHz downlink / 1713.4 MHz uplink
529| #529 : 1808.6 MHz downlink / 1713.6 MHz uplink
530| #530 : 1808.8 MHz downlink / 1713.8 MHz uplink
531| #531 : 1809 MHz downlink / 1714 MHz uplink
532| #532 : 1809.2 MHz downlink / 1714.2 MHz uplink
533| #533 : 1809.4 MHz downlink / 1714.4 MHz uplink
534| #534 : 1809.6 MHz downlink / 1714.6 MHz uplink
535| #535 : 1809.8 MHz downlink / 1714.8 MHz uplink
536| #536 : 1810 MHz downlink / 1715 MHz uplink
537| #537 : 1810.2 MHz downlink / 1715.2 MHz uplink
538| #538 : 1810.4 MHz downlink / 1715.4 MHz uplink
539| #539 : 1810.6 MHz downlink / 1715.6 MHz uplink
540| #540 : 1810.8 MHz downlink / 1715.8 MHz uplink
541| #541 : 1811 MHz downlink / 1716 MHz uplink
542| #542 : 1811.2 MHz downlink / 1716.2 MHz uplink
543| #543 : 1811.4 MHz downlink / 1716.4 MHz uplink
544| #544 : 1811.6 MHz downlink / 1716.6 MHz uplink
545| #545 : 1811.8 MHz downlink / 1716.8 MHz uplink
546| #546 : 1812 MHz downlink / 1717 MHz uplink
547| #547 : 1812.2 MHz downlink / 1717.2 MHz uplink
548| #548 : 1812.4 MHz downlink / 1717.4 MHz uplink
549| #549 : 1812.6 MHz downlink / 1717.6 MHz uplink
550| #550 : 1812.8 MHz downlink / 1717.8 MHz uplink
551| #551 : 1813 MHz downlink / 1718 MHz uplink
552| #552 : 1813.2 MHz downlink / 1718.2 MHz uplink
553| #553 : 1813.4 MHz downlink / 1718.4 MHz uplink
554| #554 : 1813.6 MHz downlink / 1718.6 MHz uplink
555| #555 : 1813.8 MHz downlink / 1718.8 MHz uplink
556| #556 : 1814 MHz downlink / 1719 MHz uplink
557| #557 : 1814.2 MHz downlink / 1719.2 MHz uplink
558| #558 : 1814.4 MHz downlink / 1719.4 MHz uplink
559| #559 : 1814.6 MHz downlink / 1719.6 MHz uplink
560| #560 : 1814.8 MHz downlink / 1719.8 MHz uplink
561| #561 : 1815 MHz downlink / 1720 MHz uplink
562| #562 : 1815.2 MHz downlink / 1720.2 MHz uplink
563| #563 : 1815.4 MHz downlink / 1720.4 MHz uplink
564| #564 : 1815.6 MHz downlink / 1720.6 MHz uplink
565| #565 : 1815.8 MHz downlink / 1720.8 MHz uplink
566| #566 : 1816 MHz downlink / 1721 MHz uplink
567| #567 : 1816.2 MHz downlink / 1721.2 MHz uplink
568| #568 : 1816.4 MHz downlink / 1721.4 MHz uplink
569| #569 : 1816.6 MHz downlink / 1721.6 MHz uplink
570| #570 : 1816.8 MHz downlink / 1721.8 MHz uplink
571| #571 : 1817 MHz downlink / 1722 MHz uplink
572| #572 : 1817.2 MHz downlink / 1722.2 MHz uplink
573| #573 : 1817.4 MHz downlink / 1722.4 MHz uplink
574| #574 : 1817.6 MHz downlink / 1722.6 MHz uplink
575| #575 : 1817.8 MHz downlink / 1722.8 MHz uplink
576| #576 : 1818 MHz downlink / 1723 MHz uplink
577| #577 : 1818.2 MHz downlink / 1723.2 MHz uplink
578| #578 : 1818.4 MHz downlink / 1723.4 MHz uplink
579| #579 : 1818.6 MHz downlink / 1723.6 MHz uplink
580| #580 : 1818.8 MHz downlink / 1723.8 MHz uplink
581| #581 : 1819 MHz downlink / 1724 MHz uplink
582| #582 : 1819.2 MHz downlink / 1724.2 MHz uplink
583| #583 : 1819.4 MHz downlink / 1724.4 MHz uplink
584| #584 : 1819.6 MHz downlink / 1724.6 MHz uplink
585| #585 : 1819.8 MHz downlink / 1724.8 MHz uplink
586| #586 : 1820 MHz downlink / 1725 MHz uplink
587| #587 : 1820.2 MHz downlink / 1725.2 MHz uplink
588| #588 : 1820.4 MHz downlink / 1725.4 MHz uplink
589| #589 : 1820.6 MHz downlink / 1725.6 MHz uplink
590| #590 : 1820.8 MHz downlink / 1725.8 MHz uplink
591| #591 : 1821 MHz downlink / 1726 MHz uplink
592| #592 : 1821.2 MHz downlink / 1726.2 MHz uplink
593| #593 : 1821.4 MHz downlink / 1726.4 MHz uplink
594| #594 : 1821.6 MHz downlink / 1726.6 MHz uplink
595| #595 : 1821.8 MHz downlink / 1726.8 MHz uplink
596| #596 : 1822 MHz downlink / 1727 MHz uplink
597| #597 : 1822.2 MHz downlink / 1727.2 MHz uplink
598| #598 : 1822.4 MHz downlink / 1727.4 MHz uplink
599| #599 : 1822.6 MHz downlink / 1727.6 MHz uplink
600| #600 : 1822.8 MHz downlink / 1727.8 MHz uplink
601| #601 : 1823 MHz downlink / 1728 MHz uplink
602| #602 : 1823.2 MHz downlink / 1728.2 MHz uplink
603| #603 : 1823.4 MHz downlink / 1728.4 MHz uplink
604| #604 : 1823.6 MHz downlink / 1728.6 MHz uplink
605| #605 : 1823.8 MHz downlink / 1728.8 MHz uplink
606| #606 : 1824 MHz downlink / 1729 MHz uplink
607| #607 : 1824.2 MHz downlink / 1729.2 MHz uplink
608| #608 : 1824.4 MHz downlink / 1729.4 MHz uplink
609| #609 : 1824.6 MHz downlink / 1729.6 MHz uplink
610| #610 : 1824.8 MHz downlink / 1729.8 MHz uplink
611| #611 : 1825 MHz downlink / 1730 MHz uplink
612| #612 : 1825.2 MHz downlink / 1730.2 MHz uplink
613| #613 : 1825.4 MHz downlink / 1730.4 MHz uplink
614| #614 : 1825.59 MHz downlink / 1730.59 MHz uplink
615| #615 : 1825.79 MHz downlink / 1730.79 MHz uplink
616| #616 : 1825.99 MHz downlink / 1730.99 MHz uplink
617| #617 : 1826.19 MHz downlink / 1731.19 MHz uplink
618| #618 : 1826.39 MHz downlink / 1731.39 MHz uplink
619| #619 : 1826.59 MHz downlink / 1731.59 MHz uplink
620| #620 : 1826.79 MHz downlink / 1731.79 MHz uplink
621| #621 : 1826.99 MHz downlink / 1731.99 MHz uplink
622| #622 : 1827.19 MHz downlink / 1732.19 MHz uplink
623| #623 : 1827.39 MHz downlink / 1732.39 MHz uplink
624| #624 : 1827.59 MHz downlink / 1732.59 MHz uplink
625| #625 : 1827.79 MHz downlink / 1732.79 MHz uplink
626| #626 : 1827.99 MHz downlink / 1732.99 MHz uplink
627| #627 : 1828.19 MHz downlink / 1733.19 MHz uplink
628| #628 : 1828.39 MHz downlink / 1733.39 MHz uplink
629| #629 : 1828.59 MHz downlink / 1733.59 MHz uplink
630| #630 : 1828.79 MHz downlink / 1733.79 MHz uplink
631| #631 : 1828.99 MHz downlink / 1733.99 MHz uplink
632| #632 : 1829.19 MHz downlink / 1734.19 MHz uplink
633| #633 : 1829.39 MHz downlink / 1734.39 MHz uplink
634| #634 : 1829.59 MHz downlink / 1734.59 MHz uplink
635| #635 : 1829.79 MHz downlink / 1734.79 MHz uplink
636| #636 : 1829.99 MHz downlink / 1734.99 MHz uplink
637| #637 : 1830.19 MHz downlink / 1735.19 MHz uplink
638| #638 : 1830.39 MHz downlink / 1735.39 MHz uplink
639| #639 : 1830.59 MHz downlink / 1735.59 MHz uplink
640| #640 : 1830.79 MHz downlink / 1735.79 MHz uplink
641| #641 : 1830.99 MHz downlink / 1735.99 MHz uplink
642| #642 : 1831.19 MHz downlink / 1736.19 MHz uplink
643| #643 : 1831.39 MHz downlink / 1736.39 MHz uplink
644| #644 : 1831.59 MHz downlink / 1736.59 MHz uplink
645| #645 : 1831.79 MHz downlink / 1736.79 MHz uplink
646| #646 : 1831.99 MHz downlink / 1736.99 MHz uplink
647| #647 : 1832.19 MHz downlink / 1737.19 MHz uplink
648| #648 : 1832.39 MHz downlink / 1737.39 MHz uplink
649| #649 : 1832.59 MHz downlink / 1737.59 MHz uplink
650| #650 : 1832.79 MHz downlink / 1737.79 MHz uplink
651| #651 : 1832.99 MHz downlink / 1737.99 MHz uplink
652| #652 : 1833.19 MHz downlink / 1738.19 MHz uplink
653| #653 : 1833.39 MHz downlink / 1738.39 MHz uplink
654| #654 : 1833.59 MHz downlink / 1738.59 MHz uplink
655| #655 : 1833.79 MHz downlink / 1738.79 MHz uplink
656| #656 : 1833.99 MHz downlink / 1738.99 MHz uplink
657| #657 : 1834.19 MHz downlink / 1739.19 MHz uplink
658| #658 : 1834.39 MHz downlink / 1739.39 MHz uplink
659| #659 : 1834.59 MHz downlink / 1739.59 MHz uplink
660| #660 : 1834.79 MHz downlink / 1739.79 MHz uplink
661| #661 : 1834.99 MHz downlink / 1739.99 MHz uplink
662| #662 : 1835.19 MHz downlink / 1740.19 MHz uplink
663| #663 : 1835.39 MHz downlink / 1740.39 MHz uplink
664| #664 : 1835.59 MHz downlink / 1740.59 MHz uplink
665| #665 : 1835.79 MHz downlink / 1740.79 MHz uplink
666| #666 : 1835.99 MHz downlink / 1740.99 MHz uplink
667| #667 : 1836.19 MHz downlink / 1741.19 MHz uplink
668| #668 : 1836.39 MHz downlink / 1741.39 MHz uplink
669| #669 : 1836.59 MHz downlink / 1741.59 MHz uplink
670| #670 : 1836.79 MHz downlink / 1741.79 MHz uplink
671| #671 : 1836.99 MHz downlink / 1741.99 MHz uplink
672| #672 : 1837.19 MHz downlink / 1742.19 MHz uplink
673| #673 : 1837.39 MHz downlink / 1742.39 MHz uplink
674| #674 : 1837.59 MHz downlink / 1742.59 MHz uplink
675| #675 : 1837.79 MHz downlink / 1742.79 MHz uplink
676| #676 : 1837.99 MHz downlink / 1742.99 MHz uplink
677| #677 : 1838.19 MHz downlink / 1743.19 MHz uplink
678| #678 : 1838.39 MHz downlink / 1743.39 MHz uplink
679| #679 : 1838.59 MHz downlink / 1743.59 MHz uplink
680| #680 : 1838.79 MHz downlink / 1743.79 MHz uplink
681| #681 : 1838.99 MHz downlink / 1743.99 MHz uplink
682| #682 : 1839.19 MHz downlink / 1744.19 MHz uplink
683| #683 : 1839.39 MHz downlink / 1744.39 MHz uplink
684| #684 : 1839.59 MHz downlink / 1744.59 MHz uplink
685| #685 : 1839.79 MHz downlink / 1744.79 MHz uplink
686| #686 : 1839.99 MHz downlink / 1744.99 MHz uplink
687| #687 : 1840.19 MHz downlink / 1745.19 MHz uplink
688| #688 : 1840.39 MHz downlink / 1745.39 MHz uplink
689| #689 : 1840.59 MHz downlink / 1745.59 MHz uplink
690| #690 : 1840.79 MHz downlink / 1745.79 MHz uplink
691| #691 : 1840.99 MHz downlink / 1745.99 MHz uplink
692| #692 : 1841.19 MHz downlink / 1746.19 MHz uplink
693| #693 : 1841.39 MHz downlink / 1746.39 MHz uplink
694| #694 : 1841.59 MHz downlink / 1746.59 MHz uplink
695| #695 : 1841.79 MHz downlink / 1746.79 MHz uplink
696| #696 : 1841.99 MHz downlink / 1746.99 MHz uplink
697| #697 : 1842.19 MHz downlink / 1747.19 MHz uplink
698| #698 : 1842.39 MHz downlink / 1747.39 MHz uplink
699| #699 : 1842.59 MHz downlink / 1747.59 MHz uplink
700| #700 : 1842.79 MHz downlink / 1747.79 MHz uplink
701| #701 : 1842.99 MHz downlink / 1747.99 MHz uplink
702| #702 : 1843.19 MHz downlink / 1748.19 MHz uplink
703| #703 : 1843.39 MHz downlink / 1748.39 MHz uplink
704| #704 : 1843.59 MHz downlink / 1748.59 MHz uplink
705| #705 : 1843.79 MHz downlink / 1748.79 MHz uplink
706| #706 : 1843.99 MHz downlink / 1748.99 MHz uplink
707| #707 : 1844.19 MHz downlink / 1749.19 MHz uplink
708| #708 : 1844.39 MHz downlink / 1749.39 MHz uplink
709| #709 : 1844.59 MHz downlink / 1749.59 MHz uplink
710| #710 : 1844.79 MHz downlink / 1749.79 MHz uplink
711| #711 : 1844.99 MHz downlink / 1749.99 MHz uplink
712| #712 : 1845.19 MHz downlink / 1750.19 MHz uplink
713| #713 : 1845.39 MHz downlink / 1750.39 MHz uplink
714| #714 : 1845.59 MHz downlink / 1750.59 MHz uplink
715| #715 : 1845.79 MHz downlink / 1750.79 MHz uplink
716| #716 : 1845.99 MHz downlink / 1750.99 MHz uplink
717| #717 : 1846.19 MHz downlink / 1751.19 MHz uplink
718| #718 : 1846.39 MHz downlink / 1751.39 MHz uplink
719| #719 : 1846.59 MHz downlink / 1751.59 MHz uplink
720| #720 : 1846.79 MHz downlink / 1751.79 MHz uplink
721| #721 : 1846.99 MHz downlink / 1751.99 MHz uplink
722| #722 : 1847.19 MHz downlink / 1752.19 MHz uplink
723| #723 : 1847.39 MHz downlink / 1752.39 MHz uplink
724| #724 : 1847.59 MHz downlink / 1752.59 MHz uplink
725| #725 : 1847.79 MHz downlink / 1752.79 MHz uplink
726| #726 : 1847.99 MHz downlink / 1752.99 MHz uplink
727| #727 : 1848.19 MHz downlink / 1753.19 MHz uplink
728| #728 : 1848.39 MHz downlink / 1753.39 MHz uplink
729| #729 : 1848.59 MHz downlink / 1753.59 MHz uplink
730| #730 : 1848.79 MHz downlink / 1753.79 MHz uplink
731| #731 : 1848.99 MHz downlink / 1753.99 MHz uplink
732| #732 : 1849.19 MHz downlink / 1754.19 MHz uplink
733| #733 : 1849.39 MHz downlink / 1754.39 MHz uplink
734| #734 : 1849.59 MHz downlink / 1754.59 MHz uplink
735| #735 : 1849.79 MHz downlink / 1754.79 MHz uplink
736| #736 : 1849.99 MHz downlink / 1754.99 MHz uplink
737| #737 : 1850.19 MHz downlink / 1755.19 MHz uplink
738| #738 : 1850.39 MHz downlink / 1755.39 MHz uplink
739| #739 : 1850.59 MHz downlink / 1755.59 MHz uplink
740| #740 : 1850.79 MHz downlink / 1755.79 MHz uplink
741| #741 : 1850.99 MHz downlink / 1755.99 MHz uplink
742| #742 : 1851.19 MHz downlink / 1756.19 MHz uplink
743| #743 : 1851.39 MHz downlink / 1756.39 MHz uplink
744| #744 : 1851.59 MHz downlink / 1756.59 MHz uplink
745| #745 : 1851.79 MHz downlink / 1756.79 MHz uplink
746| #746 : 1851.99 MHz downlink / 1756.99 MHz uplink
747| #747 : 1852.19 MHz downlink / 1757.19 MHz uplink
748| #748 : 1852.39 MHz downlink / 1757.39 MHz uplink
749| #749 : 1852.59 MHz downlink / 1757.59 MHz uplink
750| #750 : 1852.79 MHz downlink / 1757.79 MHz uplink
751| #751 : 1852.99 MHz downlink / 1757.99 MHz uplink
752| #752 : 1853.19 MHz downlink / 1758.19 MHz uplink
753| #753 : 1853.39 MHz downlink / 1758.39 MHz uplink
754| #754 : 1853.59 MHz downlink / 1758.59 MHz uplink
755| #755 : 1853.79 MHz downlink / 1758.79 MHz uplink
756| #756 : 1853.99 MHz downlink / 1758.99 MHz uplink
757| #757 : 1854.19 MHz downlink / 1759.19 MHz uplink
758| #758 : 1854.39 MHz downlink / 1759.39 MHz uplink
759| #759 : 1854.59 MHz downlink / 1759.59 MHz uplink
760| #760 : 1854.79 MHz downlink / 1759.79 MHz uplink
761| #761 : 1854.99 MHz downlink / 1759.99 MHz uplink
762| #762 : 1855.19 MHz downlink / 1760.19 MHz uplink
763| #763 : 1855.39 MHz downlink / 1760.39 MHz uplink
764| #764 : 1855.59 MHz downlink / 1760.59 MHz uplink
765| #765 : 1855.79 MHz downlink / 1760.79 MHz uplink
766| #766 : 1855.99 MHz downlink / 1760.99 MHz uplink
767| #767 : 1856.19 MHz downlink / 1761.19 MHz uplink
768| #768 : 1856.39 MHz downlink / 1761.39 MHz uplink
769| #769 : 1856.59 MHz downlink / 1761.59 MHz uplink
770| #770 : 1856.79 MHz downlink / 1761.79 MHz uplink
771| #771 : 1856.99 MHz downlink / 1761.99 MHz uplink
772| #772 : 1857.19 MHz downlink / 1762.19 MHz uplink
773| #773 : 1857.39 MHz downlink / 1762.39 MHz uplink
774| #774 : 1857.59 MHz downlink / 1762.59 MHz uplink
775| #775 : 1857.79 MHz downlink / 1762.79 MHz uplink
776| #776 : 1857.99 MHz downlink / 1762.99 MHz uplink
777| #777 : 1858.19 MHz downlink / 1763.19 MHz uplink
778| #778 : 1858.39 MHz downlink / 1763.39 MHz uplink
779| #779 : 1858.59 MHz downlink / 1763.59 MHz uplink
780| #780 : 1858.79 MHz downlink / 1763.79 MHz uplink
781| #781 : 1858.99 MHz downlink / 1763.99 MHz uplink
782| #782 : 1859.19 MHz downlink / 1764.19 MHz uplink
783| #783 : 1859.39 MHz downlink / 1764.39 MHz uplink
784| #784 : 1859.59 MHz downlink / 1764.59 MHz uplink
785| #785 : 1859.79 MHz downlink / 1764.79 MHz uplink
786| #786 : 1859.99 MHz downlink / 1764.99 MHz uplink
787| #787 : 1860.19 MHz downlink / 1765.19 MHz uplink
788| #788 : 1860.39 MHz downlink / 1765.39 MHz uplink
789| #789 : 1860.59 MHz downlink / 1765.59 MHz uplink
790| #790 : 1860.79 MHz downlink / 1765.79 MHz uplink
791| #791 : 1860.99 MHz downlink / 1765.99 MHz uplink
792| #792 : 1861.19 MHz downlink / 1766.19 MHz uplink
793| #793 : 1861.39 MHz downlink / 1766.39 MHz uplink
794| #794 : 1861.59 MHz downlink / 1766.59 MHz uplink
795| #795 : 1861.79 MHz downlink / 1766.79 MHz uplink
796| #796 : 1861.99 MHz downlink / 1766.99 MHz uplink
797| #797 : 1862.19 MHz downlink / 1767.19 MHz uplink
798| #798 : 1862.39 MHz downlink / 1767.39 MHz uplink
799| #799 : 1862.59 MHz downlink / 1767.59 MHz uplink
800| #800 : 1862.79 MHz downlink / 1767.79 MHz uplink
801| #801 : 1862.99 MHz downlink / 1767.99 MHz uplink
802| #802 : 1863.19 MHz downlink / 1768.19 MHz uplink
803| #803 : 1863.39 MHz downlink / 1768.39 MHz uplink
804| #804 : 1863.59 MHz downlink / 1768.59 MHz uplink
805| #805 : 1863.79 MHz downlink / 1768.79 MHz uplink
806| #806 : 1863.99 MHz downlink / 1768.99 MHz uplink
807| #807 : 1864.19 MHz downlink / 1769.19 MHz uplink
808| #808 : 1864.39 MHz downlink / 1769.39 MHz uplink
809| #809 : 1864.59 MHz downlink / 1769.59 MHz uplink
810| #810 : 1864.79 MHz downlink / 1769.79 MHz uplink
811| #811 : 1864.99 MHz downlink / 1769.99 MHz uplink
812| #812 : 1865.19 MHz downlink / 1770.19 MHz uplink
813| #813 : 1865.39 MHz downlink / 1770.39 MHz uplink
814| #814 : 1865.59 MHz downlink / 1770.59 MHz uplink
815| #815 : 1865.79 MHz downlink / 1770.79 MHz uplink
816| #816 : 1865.99 MHz downlink / 1770.99 MHz uplink
817| #817 : 1866.19 MHz downlink / 1771.19 MHz uplink
818| #818 : 1866.39 MHz downlink / 1771.39 MHz uplink
819| #819 : 1866.58 MHz downlink / 1771.58 MHz uplink
820| #820 : 1866.78 MHz downlink / 1771.78 MHz uplink
821| #821 : 1866.98 MHz downlink / 1771.98 MHz uplink
822| #822 : 1867.18 MHz downlink / 1772.18 MHz uplink
823| #823 : 1867.38 MHz downlink / 1772.38 MHz uplink
824| #824 : 1867.58 MHz downlink / 1772.58 MHz uplink
825| #825 : 1867.78 MHz downlink / 1772.78 MHz uplink
826| #826 : 1867.98 MHz downlink / 1772.98 MHz uplink
827| #827 : 1868.18 MHz downlink / 1773.18 MHz uplink
828| #828 : 1868.38 MHz downlink / 1773.38 MHz uplink
829| #829 : 1868.58 MHz downlink / 1773.58 MHz uplink
830| #830 : 1868.78 MHz downlink / 1773.78 MHz uplink
831| #831 : 1868.98 MHz downlink / 1773.98 MHz uplink
832| #832 : 1869.18 MHz downlink / 1774.18 MHz uplink
833| #833 : 1869.38 MHz downlink / 1774.38 MHz uplink
834| #834 : 1869.58 MHz downlink / 1774.58 MHz uplink
835| #835 : 1869.78 MHz downlink / 1774.78 MHz uplink
836| #836 : 1869.98 MHz downlink / 1774.98 MHz uplink
837| #837 : 1870.18 MHz downlink / 1775.18 MHz uplink
838| #838 : 1870.38 MHz downlink / 1775.38 MHz uplink
839| #839 : 1870.58 MHz downlink / 1775.58 MHz uplink
840| #840 : 1870.78 MHz downlink / 1775.78 MHz uplink
841| #841 : 1870.98 MHz downlink / 1775.98 MHz uplink
842| #842 : 1871.18 MHz downlink / 1776.18 MHz uplink
843| #843 : 1871.38 MHz downlink / 1776.38 MHz uplink
844| #844 : 1871.58 MHz downlink / 1776.58 MHz uplink
845| #845 : 1871.78 MHz downlink / 1776.78 MHz uplink
846| #846 : 1871.98 MHz downlink / 1776.98 MHz uplink
847| #847 : 1872.18 MHz downlink / 1777.18 MHz uplink
848| #848 : 1872.38 MHz downlink / 1777.38 MHz uplink
849| #849 : 1872.58 MHz downlink / 1777.58 MHz uplink
850| #850 : 1872.78 MHz downlink / 1777.78 MHz uplink
851| #851 : 1872.98 MHz downlink / 1777.98 MHz uplink
852| #852 : 1873.18 MHz downlink / 1778.18 MHz uplink
853| #853 : 1873.38 MHz downlink / 1778.38 MHz uplink
854| #854 : 1873.58 MHz downlink / 1778.58 MHz uplink
855| #855 : 1873.78 MHz downlink / 1778.78 MHz uplink
856| #856 : 1873.98 MHz downlink / 1778.98 MHz uplink
857| #857 : 1874.18 MHz downlink / 1779.18 MHz uplink
858| #858 : 1874.38 MHz downlink / 1779.38 MHz uplink
859| #859 : 1874.58 MHz downlink / 1779.58 MHz uplink
860| #860 : 1874.78 MHz downlink / 1779.78 MHz uplink
861| #861 : 1874.98 MHz downlink / 1779.98 MHz uplink
862| #862 : 1875.18 MHz downlink / 1780.18 MHz uplink
863| #863 : 1875.38 MHz downlink / 1780.38 MHz uplink
864| #864 : 1875.58 MHz downlink / 1780.58 MHz uplink
865| #865 : 1875.78 MHz downlink / 1780.78 MHz uplink
866| #866 : 1875.98 MHz downlink / 1780.98 MHz uplink
867| #867 : 1876.18 MHz downlink / 1781.18 MHz uplink
868| #868 : 1876.38 MHz downlink / 1781.38 MHz uplink
869| #869 : 1876.58 MHz downlink / 1781.58 MHz uplink
870| #870 : 1876.78 MHz downlink / 1781.78 MHz uplink
871| #871 : 1876.98 MHz downlink / 1781.98 MHz uplink
872| #872 : 1877.18 MHz downlink / 1782.18 MHz uplink
873| #873 : 1877.38 MHz downlink / 1782.38 MHz uplink
874| #874 : 1877.58 MHz downlink / 1782.58 MHz uplink
875| #875 : 1877.78 MHz downlink / 1782.78 MHz uplink
876| #876 : 1877.98 MHz downlink / 1782.98 MHz uplink
877| #877 : 1878.18 MHz downlink / 1783.18 MHz uplink
878| #878 : 1878.38 MHz downlink / 1783.38 MHz uplink
879| #879 : 1878.58 MHz downlink / 1783.58 MHz uplink
880| #880 : 1878.78 MHz downlink / 1783.78 MHz uplink
881| #881 : 1878.98 MHz downlink / 1783.98 MHz uplink
882| #882 : 1879.18 MHz downlink / 1784.18 MHz uplink
883| #883 : 1879.38 MHz downlink / 1784.38 MHz uplink
884| #884 : 1879.58 MHz downlink / 1784.58 MHz uplink
885| #885 : 1879.78 MHz downlink / 1784.78 MHz uplink
INVALID|PCS1900
512| #512 : 1930.2 MHz downlink / 1850.2 MHz uplink
513| #513 : 1930.4 MHz downlink / 1850.4 MHz uplink
514| #514 : 1930.6 MHz downlink / 1850.6 MHz uplink
515| #515 : 1930.8 MHz downlink / 1850.8 MHz uplink
516| #516 : 1931 MHz downlink / 1851 MHz uplink
517| #517 : 1931.2 MHz downlink / 1851.2 MHz uplink
518| #518 : 1931.4 MHz downlink / 1851.4 MHz uplink
519| #519 : 1931.6 MHz downlink / 1851.6 MHz uplink
520| #520 : 1931.8 MHz downlink / 1851.8 MHz uplink
521| #521 : 1932 MHz downlink / 1852 MHz uplink
522| #522 : 1932.2 MHz downlink / 1852.2 MHz uplink
523| #523 : 1932.4 MHz downlink / 1852.4 MHz uplink
524| #524 : 1932.6 MHz downlink / 1852.6 MHz uplink
525| #525 : 1932.8 MHz downlink / 1852.8 MHz uplink
526| #526 : 1933 MHz downlink / 1853 MHz uplink
527| #527 : 1933.2 MHz downlink / 1853.2 MHz uplink
528| #528 : 1933.4 MHz downlink / 1853.4 MHz uplink
529| #529 : 1933.6 MHz downlink / 1853.6 MHz uplink
530| #530 : 1933.8 MHz downlink / 1853.8 MHz uplink
531| #531 : 1934 MHz downlink / 1854 MHz uplink
532| #532 : 1934.2 MHz downlink / 1854.2 MHz uplink
533| #533 : 1934.4 MHz downlink / 1854.4 MHz uplink
534| #534 : 1934.6 MHz downlink / 1854.6 MHz uplink
535| #535 : 1934.8 MHz downlink / 1854.8 MHz uplink
536| #536 : 1935 MHz downlink / 1855 MHz uplink
537| #537 : 1935.2 MHz downlink / 1855.2 MHz uplink
538| #538 : 1935.4 MHz downlink / 1855.4 MHz uplink
539| #539 : 1935.6 MHz downlink / 1855.6 MHz uplink
540| #540 : 1935.8 MHz downlink / 1855.8 MHz uplink
541| #541 : 1936 MHz downlink / 1856 MHz uplink
542| #542 : 1936.2 MHz downlink / 1856.2 MHz uplink
543| #543 : 1936.4 MHz downlink / 1856.4 MHz uplink
544| #544 : 1936.6 MHz downlink / 1856.6 MHz uplink
545| #545 : 1936.8 MHz downlink / 1856.8 MHz uplink
546| #546 : 1937 MHz downlink / 1857 MHz uplink
547| #547 : 1937.2 MHz downlink / 1857.2 MHz uplink
548| #548 : 1937.4 MHz downlink / 1857.4 MHz uplink
549| #549 : 1937.6 MHz downlink / 1857.6 MHz uplink
550| #550 : 1937.8 MHz downlink / 1857.8 MHz uplink
551| #551 : 1938 MHz downlink / 1858 MHz uplink
552| #552 : 1938.2 MHz downlink / 1858.2 MHz uplink
553| #553 : 1938.4 MHz downlink / 1858.4 MHz uplink
554| #554 : 1938.6 MHz downlink / 1858.6 MHz uplink
555| #555 : 1938.8 MHz downlink / 1858.8 MHz uplink
556| #556 : 1939 MHz downlink / 1859 MHz uplink
557| #557 : 1939.2 MHz downlink / 1859.2 MHz uplink
558| #558 : 1939.4 MHz downlink / 1859.4 MHz uplink
559| #559 : 1939.6 MHz downlink / 1859.6 MHz uplink
560| #560 : 1939.8 MHz downlink / 1859.8 MHz uplink
561| #561 : 1940 MHz downlink / 1860 MHz uplink
562| #562 : 1940.2 MHz downlink / 1860.2 MHz uplink
563| #563 : 1940.4 MHz downlink / 1860.4 MHz uplink
564| #564 : 1940.6 MHz downlink / 1860.6 MHz uplink
565| #565 : 1940.8 MHz downlink / 1860.8 MHz uplink
566| #566 : 1941 MHz downlink / 1861 MHz uplink
567| #567 : 1941.2 MHz downlink / 1861.2 MHz uplink
568| #568 : 1941.4 MHz downlink / 1861.4 MHz uplink
569| #569 : 1941.6 MHz downlink / 1861.6 MHz uplink
570| #570 : 1941.8 MHz downlink / 1861.8 MHz uplink
571| #571 : 1942 MHz downlink / 1862 MHz uplink
572| #572 : 1942.2 MHz downlink / 1862.2 MHz uplink
573| #573 : 1942.4 MHz downlink / 1862.4 MHz uplink
574| #574 : 1942.6 MHz downlink / 1862.6 MHz uplink
575| #575 : 1942.8 MHz downlink / 1862.8 MHz uplink
576| #576 : 1943 MHz downlink / 1863 MHz uplink
577| #577 : 1943.2 MHz downlink / 1863.2 MHz uplink
578| #578 : 1943.4 MHz downlink / 1863.4 MHz uplink
579| #579 : 1943.6 MHz downlink / 1863.6 MHz uplink
580| #580 : 1943.8 MHz downlink / 1863.8 MHz uplink
581| #581 : 1944 MHz downlink / 1864 MHz uplink
582| #582 : 1944.2 MHz downlink / 1864.2 MHz uplink
583| #583 : 1944.4 MHz downlink / 1864.4 MHz uplink
584| #584 : 1944.6 MHz downlink / 1864.6 MHz uplink
585| #585 : 1944.8 MHz downlink / 1864.8 MHz uplink
586| #586 : 1945 MHz downlink / 1865 MHz uplink
587| #587 : 1945.2 MHz downlink / 1865.2 MHz uplink
588| #588 : 1945.4 MHz downlink / 1865.4 MHz uplink
589| #589 : 1945.6 MHz downlink / 1865.6 MHz uplink
590| #590 : 1945.8 MHz downlink / 1865.8 MHz uplink
591| #591 : 1946 MHz downlink / 1866 MHz uplink
592| #592 : 1946.2 MHz downlink / 1866.2 MHz uplink
593| #593 : 1946.4 MHz downlink / 1866.4 MHz uplink
594| #594 : 1946.6 MHz downlink / 1866.6 MHz uplink
595| #595 : 1946.8 MHz downlink / 1866.8 MHz uplink
596| #596 : 1947 MHz downlink / 1867 MHz uplink
597| #597 : 1947.2 MHz downlink / 1867.2 MHz uplink
598| #598 : 1947.4 MHz downlink / 1867.4 MHz uplink
599| #599 : 1947.6 MHz downlink / 1867.6 MHz uplink
600| #600 : 1947.8 MHz downlink / 1867.8 MHz uplink
601| #601 : 1948 MHz downlink / 1868 MHz uplink
602| #602 : 1948.2 MHz downlink / 1868.2 MHz uplink
603| #603 : 1948.4 MHz downlink / 1868.4 MHz uplink
604| #604 : 1948.6 MHz downlink / 1868.6 MHz uplink
605| #605 : 1948.8 MHz downlink / 1868.8 MHz uplink
606| #606 : 1949 MHz downlink / 1869 MHz uplink
607| #607 : 1949.2 MHz downlink / 1869.2 MHz uplink
608| #608 : 1949.4 MHz downlink / 1869.4 MHz uplink
609| #609 : 1949.6 MHz downlink / 1869.6 MHz uplink
610| #610 : 1949.8 MHz downlink / 1869.8 MHz uplink
611| #611 : 1950 MHz downlink / 1870 MHz uplink
612| #612 : 1950.2 MHz downlink / 1870.2 MHz uplink
613| #613 : 1950.4 MHz downlink / 1870.4 MHz uplink
614| #614 : 1950.59 MHz downlink / 1870.59 MHz uplink
615| #615 : 1950.79 MHz downlink / 1870.79 MHz uplink
616| #616 : 1950.99 MHz downlink / 1870.99 MHz uplink
617| #617 : 1951.19 MHz downlink / 1871.19 MHz uplink
618| #618 : 1951.39 MHz downlink / 1871.39 MHz uplink
619| #619 : 1951.59 MHz downlink / 1871.59 MHz uplink
620| #620 : 1951.79 MHz downlink / 1871.79 MHz uplink
621| #621 : 1951.99 MHz downlink / 1871.99 MHz uplink
622| #622 : 1952.19 MHz downlink / 1872.19 MHz uplink
623| #623 : 1952.39 MHz downlink / 1872.39 MHz uplink
624| #624 : 1952.59 MHz downlink / 1872.59 MHz uplink
625| #625 : 1952.79 MHz downlink / 1872.79 MHz uplink
626| #626 : 1952.99 MHz downlink / 1872.99 MHz uplink
627| #627 : 1953.19 MHz downlink / 1873.19 MHz uplink
628| #628 : 1953.39 MHz downlink / 1873.39 MHz uplink
629| #629 : 1953.59 MHz downlink / 1873.59 MHz uplink
630| #630 : 1953.79 MHz downlink / 1873.79 MHz uplink
631| #631 : 1953.99 MHz downlink / 1873.99 MHz uplink
632| #632 : 1954.19 MHz downlink / 1874.19 MHz uplink
633| #633 : 1954.39 MHz downlink / 1874.39 MHz uplink
634| #634 : 1954.59 MHz downlink / 1874.59 MHz uplink
635| #635 : 1954.79 MHz downlink / 1874.79 MHz uplink
636| #636 : 1954.99 MHz downlink / 1874.99 MHz uplink
637| #637 : 1955.19 MHz downlink / 1875.19 MHz uplink
638| #638 : 1955.39 MHz downlink / 1875.39 MHz uplink
639| #639 : 1955.59 MHz downlink / 1875.59 MHz uplink
640| #640 : 1955.79 MHz downlink / 1875.79 MHz uplink
641| #641 : 1955.99 MHz downlink / 1875.99 MHz uplink
642| #642 : 1956.19 MHz downlink / 1876.19 MHz uplink
643| #643 : 1956.39 MHz downlink / 1876.39 MHz uplink
644| #644 : 1956.59 MHz downlink / 1876.59 MHz uplink
645| #645 : 1956.79 MHz downlink / 1876.79 MHz uplink
646| #646 : 1956.99 MHz downlink / 1876.99 MHz uplink
647| #647 : 1957.19 MHz downlink / 1877.19 MHz uplink
648| #648 : 1957.39 MHz downlink / 1877.39 MHz uplink
649| #649 : 1957.59 MHz downlink / 1877.59 MHz uplink
650| #650 : 1957.79 MHz downlink / 1877.79 MHz uplink
651| #651 : 1957.99 MHz downlink / 1877.99 MHz uplink
652| #652 : 1958.19 MHz downlink / 1878.19 MHz uplink
653| #653 : 1958.39 MHz downlink / 1878.39 MHz uplink
654| #654 : 1958.59 MHz downlink / 1878.59 MHz uplink
655| #655 : 1958.79 MHz downlink / 1878.79 MHz uplink
656| #656 : 1958.99 MHz downlink / 1878.99 MHz uplink
657| #657 : 1959.19 MHz downlink / 1879.19 MHz uplink
658| #658 : 1959.39 MHz downlink / 1879.39 MHz uplink
659| #659 : 1959.59 MHz downlink / 1879.59 MHz uplink
660| #660 : 1959.79 MHz downlink / 1879.79 MHz uplink
661| #661 : 1959.99 MHz downlink / 1879.99 MHz uplink
662| #662 : 1960.19 MHz downlink / 1880.19 MHz uplink
663| #663 : 1960.39 MHz downlink / 1880.39 MHz uplink
664| #664 : 1960.59 MHz downlink / 1880.59 MHz uplink
665| #665 : 1960.79 MHz downlink / 1880.79 MHz uplink
666| #666 : 1960.99 MHz downlink / 1880.99 MHz uplink
667| #667 : 1961.19 MHz downlink / 1881.19 MHz uplink
668| #668 : 1961.39 MHz downlink / 1881.39 MHz uplink
669| #669 : 1961.59 MHz downlink / 1881.59 MHz uplink
670| #670 : 1961.79 MHz downlink / 1881.79 MHz uplink
671| #671 : 1961.99 MHz downlink / 1881.99 MHz uplink
672| #672 : 1962.19 MHz downlink / 1882.19 MHz uplink
673| #673 : 1962.39 MHz downlink / 1882.39 MHz uplink
674| #674 : 1962.59 MHz downlink / 1882.59 MHz uplink
675| #675 : 1962.79 MHz downlink / 1882.79 MHz uplink
676| #676 : 1962.99 MHz downlink / 1882.99 MHz uplink
677| #677 : 1963.19 MHz downlink / 1883.19 MHz uplink
678| #678 : 1963.39 MHz downlink / 1883.39 MHz uplink
679| #679 : 1963.59 MHz downlink / 1883.59 MHz uplink
680| #680 : 1963.79 MHz downlink / 1883.79 MHz uplink
681| #681 : 1963.99 MHz downlink / 1883.99 MHz uplink
682| #682 : 1964.19 MHz downlink / 1884.19 MHz uplink
683| #683 : 1964.39 MHz downlink / 1884.39 MHz uplink
684| #684 : 1964.59 MHz downlink / 1884.59 MHz uplink
685| #685 : 1964.79 MHz downlink / 1884.79 MHz uplink
686| #686 : 1964.99 MHz downlink / 1884.99 MHz uplink
687| #687 : 1965.19 MHz downlink / 1885.19 MHz uplink
688| #688 : 1965.39 MHz downlink / 1885.39 MHz uplink
689| #689 : 1965.59 MHz downlink / 1885.59 MHz uplink
690| #690 : 1965.79 MHz downlink / 1885.79 MHz uplink
691| #691 : 1965.99 MHz downlink / 1885.99 MHz uplink
692| #692 : 1966.19 MHz downlink / 1886.19 MHz uplink
693| #693 : 1966.39 MHz downlink / 1886.39 MHz uplink
694| #694 : 1966.59 MHz downlink / 1886.59 MHz uplink
695| #695 : 1966.79 MHz downlink / 1886.79 MHz uplink
696| #696 : 1966.99 MHz downlink / 1886.99 MHz uplink
697| #697 : 1967.19 MHz downlink / 1887.19 MHz uplink
698| #698 : 1967.39 MHz downlink / 1887.39 MHz uplink
699| #699 : 1967.59 MHz downlink / 1887.59 MHz uplink
700| #700 : 1967.79 MHz downlink / 1887.79 MHz uplink
701| #701 : 1967.99 MHz downlink / 1887.99 MHz uplink
702| #702 : 1968.19 MHz downlink / 1888.19 MHz uplink
703| #703 : 1968.39 MHz downlink / 1888.39 MHz uplink
704| #704 : 1968.59 MHz downlink / 1888.59 MHz uplink
705| #705 : 1968.79 MHz downlink / 1888.79 MHz uplink
706| #706 : 1968.99 MHz downlink / 1888.99 MHz uplink
707| #707 : 1969.19 MHz downlink / 1889.19 MHz uplink
708| #708 : 1969.39 MHz downlink / 1889.39 MHz uplink
709| #709 : 1969.59 MHz downlink / 1889.59 MHz uplink
710| #710 : 1969.79 MHz downlink / 1889.79 MHz uplink
711| #711 : 1969.99 MHz downlink / 1889.99 MHz uplink
712| #712 : 1970.19 MHz downlink / 1890.19 MHz uplink
713| #713 : 1970.39 MHz downlink / 1890.39 MHz uplink
714| #714 : 1970.59 MHz downlink / 1890.59 MHz uplink
715| #715 : 1970.79 MHz downlink / 1890.79 MHz uplink
716| #716 : 1970.99 MHz downlink / 1890.99 MHz uplink
717| #717 : 1971.19 MHz downlink / 1891.19 MHz uplink
718| #718 : 1971.39 MHz downlink / 1891.39 MHz uplink
719| #719 : 1971.59 MHz downlink / 1891.59 MHz uplink
720| #720 : 1971.79 MHz downlink / 1891.79 MHz uplink
721| #721 : 1971.99 MHz downlink / 1891.99 MHz uplink
722| #722 : 1972.19 MHz downlink / 1892.19 MHz uplink
723| #723 : 1972.39 MHz downlink / 1892.39 MHz uplink
724| #724 : 1972.59 MHz downlink / 1892.59 MHz uplink
725| #725 : 1972.79 MHz downlink / 1892.79 MHz uplink
726| #726 : 1972.99 MHz downlink / 1892.99 MHz uplink
727| #727 : 1973.19 MHz downlink / 1893.19 MHz uplink
728| #728 : 1973.39 MHz downlink / 1893.39 MHz uplink
729| #729 : 1973.59 MHz downlink / 1893.59 MHz uplink
730| #730 : 1973.79 MHz downlink / 1893.79 MHz uplink
731| #731 : 1973.99 MHz downlink / 1893.99 MHz uplink
732| #732 : 1974.19 MHz downlink / 1894.19 MHz uplink
733| #733 : 1974.39 MHz downlink / 1894.39 MHz uplink
734| #734 : 1974.59 MHz downlink / 1894.59 MHz uplink
735| #735 : 1974.79 MHz downlink / 1894.79 MHz uplink
736| #736 : 1974.99 MHz downlink / 1894.99 MHz uplink
737| #737 : 1975.19 MHz downlink / 1895.19 MHz uplink
738| #738 : 1975.39 MHz downlink / 1895.39 MHz uplink
739| #739 : 1975.59 MHz downlink / 1895.59 MHz uplink
740| #740 : 1975.79 MHz downlink / 1895.79 MHz uplink
741| #741 : 1975.99 MHz downlink / 1895.99 MHz uplink
742| #742 : 1976.19 MHz downlink / 1896.19 MHz uplink
743| #743 : 1976.39 MHz downlink / 1896.39 MHz uplink
744| #744 : 1976.59 MHz downlink / 1896.59 MHz uplink
745| #745 : 1976.79 MHz downlink / 1896.79 MHz uplink
746| #746 : 1976.99 MHz downlink / 1896.99 MHz uplink
747| #747 : 1977.19 MHz downlink / 1897.19 MHz uplink
748| #748 : 1977.39 MHz downlink / 1897.39 MHz uplink
749| #749 : 1977.59 MHz downlink / 1897.59 MHz uplink
750| #750 : 1977.79 MHz downlink / 1897.79 MHz uplink
751| #751 : 1977.99 MHz downlink / 1897.99 MHz uplink
752| #752 : 1978.19 MHz downlink / 1898.19 MHz uplink
753| #753 : 1978.39 MHz downlink / 1898.39 MHz uplink
754| #754 : 1978.59 MHz downlink / 1898.59 MHz uplink
755| #755 : 1978.79 MHz downlink / 1898.79 MHz uplink
756| #756 : 1978.99 MHz downlink / 1898.99 MHz uplink
757| #757 : 1979.19 MHz downlink / 1899.19 MHz uplink
758| #758 : 1979.39 MHz downlink / 1899.39 MHz uplink
759| #759 : 1979.59 MHz downlink / 1899.59 MHz uplink
760| #760 : 1979.79 MHz downlink / 1899.79 MHz uplink
761| #761 : 1979.99 MHz downlink / 1899.99 MHz uplink
762| #762 : 1980.19 MHz downlink / 1900.19 MHz uplink
763| #763 : 1980.39 MHz downlink / 1900.39 MHz uplink
764| #764 : 1980.59 MHz downlink / 1900.59 MHz uplink
765| #765 : 1980.79 MHz downlink / 1900.79 MHz uplink
766| #766 : 1980.99 MHz downlink / 1900.99 MHz uplink
767| #767 : 1981.19 MHz downlink / 1901.19 MHz uplink
768| #768 : 1981.39 MHz downlink / 1901.39 MHz uplink
769| #769 : 1981.59 MHz downlink / 1901.59 MHz uplink
770| #770 : 1981.79 MHz downlink / 1901.79 MHz uplink
771| #771 : 1981.99 MHz downlink / 1901.99 MHz uplink
772| #772 : 1982.19 MHz downlink / 1902.19 MHz uplink
773| #773 : 1982.39 MHz downlink / 1902.39 MHz uplink
774| #774 : 1982.59 MHz downlink / 1902.59 MHz uplink
775| #775 : 1982.79 MHz downlink / 1902.79 MHz uplink
776| #776 : 1982.99 MHz downlink / 1902.99 MHz uplink
777| #777 : 1983.19 MHz downlink / 1903.19 MHz uplink
778| #778 : 1983.39 MHz downlink / 1903.39 MHz uplink
779| #779 : 1983.59 MHz downlink / 1903.59 MHz uplink
780| #780 : 1983.79 MHz downlink / 1903.79 MHz uplink
781| #781 : 1983.99 MHz downlink / 1903.99 MHz uplink
782| #782 : 1984.19 MHz downlink / 1904.19 MHz uplink
783| #783 : 1984.39 MHz downlink / 1904.39 MHz uplink
784| #784 : 1984.59 MHz downlink / 1904.59 MHz uplink
785| #785 : 1984.79 MHz downlink / 1904.79 MHz uplink
786| #786 : 1984.99 MHz downlink / 1904.99 MHz uplink
787| #787 : 1985.19 MHz downlink / 1905.19 MHz uplink
788| #788 : 1985.39 MHz downlink / 1905.39 MHz uplink
789| #789 : 1985.59 MHz downlink / 1905.59 MHz uplink
790| #790 : 1985.79 MHz downlink / 1905.79 MHz uplink
791| #791 : 1985.99 MHz downlink / 1905.99 MHz uplink
792| #792 : 1986.19 MHz downlink / 1906.19 MHz uplink
793| #793 : 1986.39 MHz downlink / 1906.39 MHz uplink
794| #794 : 1986.59 MHz downlink / 1906.59 MHz uplink
795| #795 : 1986.79 MHz downlink / 1906.79 MHz uplink
796| #796 : 1986.99 MHz downlink / 1906.99 MHz uplink
797| #797 : 1987.19 MHz downlink / 1907.19 MHz uplink
798| #798 : 1987.39 MHz downlink / 1907.39 MHz uplink
799| #799 : 1987.59 MHz downlink / 1907.59 MHz uplink
800| #800 : 1987.79 MHz downlink / 1907.79 MHz uplink
801| #801 : 1987.99 MHz downlink / 1907.99 MHz uplink
802| #802 : 1988.19 MHz downlink / 1908.19 MHz uplink
803| #803 : 1988.39 MHz downlink / 1908.39 MHz uplink
804| #804 : 1988.59 MHz downlink / 1908.59 MHz uplink
805| #805 : 1988.79 MHz downlink / 1908.79 MHz uplink
806| #806 : 1988.99 MHz downlink / 1908.99 MHz uplink
807| #807 : 1989.19 MHz downlink / 1909.19 MHz uplink
808| #808 : 1989.39 MHz downlink / 1909.39 MHz uplink
809| #809 : 1989.59 MHz downlink / 1909.59 MHz uplink
810| #810 : 1989.79 MHz downlink / 1909.79 MHz uplink";

	$key = "Radio.C0";
	$expl_valid = explode("\n",$valid_values);
	$vals = array();
	$count_options = count($expl_valid);

	$band = array(850,900,1800,1900);
	$index_particle = 0;

	for ($j=0; $j<$count_options;$j++) {
		$val_opt = $expl_valid[$j];
		$expl_val_opt = explode("|",$val_opt);
		$opt_name = (!isset($expl_val_opt[1])) ? $expl_val_opt[0] : $expl_val_opt[1];
		$opt_name = explode("-",$opt_name);
		$opt_name = $opt_name[0]; 
		if ($expl_val_opt[0]=="INVALID") {
			$expl_val_opt[0] = "__disabled";
			$opt_name = "------- $opt_name -------";
			$particle = $band[$index_particle];
			$index_particle++;
			$vals[] = array($key."_id"=>"$expl_val_opt[0]",$key=>"$opt_name");
		} else
			$vals[] = array($key."_id"=>"$particle-$expl_val_opt[0]",$key=>"$opt_name");
	}
	return $vals;
}
?>
