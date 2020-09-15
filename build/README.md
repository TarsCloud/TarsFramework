The scripts is used for buiding TarsFramework

If you need multi-threaded compilation, you need to make the following modifications:

modify before as below:
```
    all)
        cd $BASEPATH;  cmake ..;  make
```

modify after as below:
```
    all)
        cd $BASEPATH;  cmake ..;  make -j 4
```



first, download all associated projects:
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
