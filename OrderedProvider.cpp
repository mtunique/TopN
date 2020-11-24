#include "OrderedProvider.h"

#include <fstream>
#include <tuple>
#include <vector>

using namespace std;


FileOrderedProvider::FileOrderedProvider(const vector<ifstream *> & partialOrders) : _partialOrders(partialOrders){};

int FileOrderedProvider::size()
{
    return _partialOrders.size();
}

tuple<string, int> FileOrderedProvider::pop(int index)
{
    string str;
    int cnt;
    *_partialOrders[index] >> str >> cnt;
    return {str, cnt};
}

bool FileOrderedProvider::empty(int index)
{
    return _partialOrders[index]->eof();
}
