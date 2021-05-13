# BuildType=Debug
BuildType=Release

rm build -rf && mkdir -p build
cd build 

cmake ../  -DCMAKE_BUILD_TYPE=$BuildType > cmake.log || exit 1
    echo "----------"
    echo "cmake Done"
    echo "----------"

make -j > make.log || exit 1
    echo "----------"
    echo "make Done"
    echo "----------"

cd ..

./build/server_feat -num_decoder 1 -num_hasher 1
