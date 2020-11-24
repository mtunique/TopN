#include <fstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

template <class T>
class OrderedProvider
{
public:
    virtual int size() = 0;
    virtual T pop(int index) = 0;
    virtual bool empty(int index) = 0;
};


template <class T>
class MemOrderedProvider : public OrderedProvider<T>
{
public:
    MemOrderedProvider(const vector<vector<T> *> & partialOrders) : _partialOrders(partialOrders)
    {
        _indexes = vector<size_t>(partialOrders.size(), 0);
    };

    int size() { return _partialOrders.size(); }

    T pop(int index) { return (*_partialOrders[index])[_indexes[index]++]; }

    bool empty(int index) { return _indexes[index] == _partialOrders[index]->size(); }

private:
    const vector<vector<T> *> _partialOrders;
    vector<size_t> _indexes;
};


class FileOrderedProvider : public OrderedProvider<tuple<string, int>>
{
public:
    FileOrderedProvider(const vector<ifstream *> & partialOrders);
    int size();
    tuple<string, int> pop(int index);
    bool empty(int index);

private:
    const vector<ifstream *> _partialOrders;
};