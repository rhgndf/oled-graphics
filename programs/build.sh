
CFLAGS="-O2 -lArduiPi_OLED -lgmp -fpermissive"

compile() {
 g++ $1.cpp -o $1 $CFLAGS
}

compile helloworld &
compile poweredby &
compile pi &
compile demo &

for job in `jobs -p`
do
    wait $job
done
