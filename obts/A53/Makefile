SOURCE_FILES=a5.c bits.c gea.c kasumi.c utils.c ifc.cpp
OBJECT_FILES=a5.o bits.o gea.o kasumi.o utils.o ifc.o
INCLUDE_FILES=a5.h bits.h gea.h gprs_cipher.h kasumi.h linuxlist.h utils.h

# build A5/3 library
liba53.so.1.0: ${SOURCE_FILES} ${INCLUDE_FILES} Makefile
	g++ -O3 -Wall -fPIC -c ${SOURCE_FILES}
	g++ -shared -Wl,-soname,liba53.so.1 -o liba53.so.1.0 ${OBJECT_FILES}
	ln -sf liba53.so.1.0 liba53.so.1
	ln -sf liba53.so.1.0 liba53.so

# install A5/3 library
install: liba53.so.1.0
	cp liba53.so.1.0 /usr/lib
	ln -sf /usr/lib/liba53.so.1.0 /usr/lib/liba53.so.1
	ln -sf /usr/lib/liba53.so.1.0 /usr/lib/liba53.so
	cp a53.h /usr/include

# test installed A5/3 library
installtest: install
	g++ -o a53test a53test.cpp -I/usr/include -L/usr/lib -la53
	./a53test

clean:
	rm -f ${OBJECT_FILES} liba53.so* a53test

# otest: a5_test kasumi_test gea_test a5_speed
# 	./a5_test | diff - a5_test.ok
# 	# ./kasumi_test | diff - kasumi_test.ok
# 	# ./gea_test | diff - gea_test.ok
# 	./a5_speed
# 
# a5_speed: a5_speed.c ${SOURCE_FILES} ${INCLUDE_FILES} Makefile
# 	g++ ${OPT} -o a5_speed a5_speed.c ${SOURCE_FILES}
# 
# a5_test: a5_test.c ${SOURCE_FILES} ${INCLUDE_FILES} Makefile
# 	g++ ${OPT} -o a5_test a5_test.c ${SOURCE_FILES}
# 
# # kasumi_test: kasumi_test.c ${SOURCE_FILES} ${INCLUDE_FILES} Makefile
# 	# g++ ${OPT} -o kasumi_test kasumi_test.c ${SOURCE_FILES}
# 
# # gea_test: gea_test.c ${SOURCE_FILES} ${INCLUDE_FILES} Makefile
# 	# g++ ${OPT} -o gea_test gea_test.c ${SOURCE_FILES}
# 
# clean:
# 	rm a5_test kasumi_test gea_test a5_speed
# 
# 
# 
