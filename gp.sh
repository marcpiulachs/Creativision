PLATFORM=`gcc --version | grep gcc`
echo 'const char *PLATFORM="'$PLATFORM'";' > $1

