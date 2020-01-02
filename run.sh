rm -rf a.out logs
g++ -std=c++11 $(find . -name '*.cpp') -DELPP_DEFAULT_LOG_FILE='"./logs/default.log"'
