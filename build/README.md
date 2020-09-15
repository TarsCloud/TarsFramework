The scripts is used for buiding TarsFramework

If you need multi-threaded compilation, you need to make the following modifications:
before modify
```
    all)
        cd $BASEPATH;  cmake ..;  make
```
after:
```
    all)
        cd $BASEPATH;  cmake ..;  make -j 4
```


download all associated projects firstly
```
build.sh prepare
```
compile
```
build.sh all
```
cleanup
```
build.sh cleanall
```
install
```
build.sh install
```
