#include "OrderedProvider.h"

#include <algorithm>
#include <execution>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace std;

const int N = 100;
const size_t MEM_SIZE = 1024 * 1024 * 1000;
const string TMP_FILE_DIR = "/tmp/top_n";

using UrlCnt = tuple<string, int>;
using StrToBucket = tuple<string, int>;

struct LessThan
{
    bool operator()(UrlCnt const & lhs, UrlCnt const & rhs) const { return std::get<1>(lhs) > std::get<1>(rhs); }
};

using Heap = priority_queue<UrlCnt, vector<UrlCnt>, LessThan>;


unordered_map<string, int> * wordCount(vector<string> * lines)
{
    unordered_map<string, int> * result = new unordered_map<string, int>();

    for (auto line = lines->begin(); line != lines->end(); line++)
    {
        if (result->contains(*line))
        {
            (*result)[*line] = (*result)[*line] + 1;
        }
        else
        {
            (*result)[*line] = 1;
        }
    }

    return result;
}

void prtialTopN(const vector<vector<string> *> & lines, int fileIndex)
{
    // multi thread word count
    vector<future<unordered_map<string, int> *>> futures;
    for (auto subLines = lines.begin(); subLines != lines.end(); subLines++)
    {
        auto f = async(wordCount, *subLines);
        futures.push_back(move(f));
    }

    vector<unordered_map<string, int> *> subCnt;
    for (auto i = futures.begin(); i != futures.end(); i++)
    {
        subCnt.push_back(i->get());
    }


    // merge word count result
    vector<string> result;
    for (size_t i = 0; i < subCnt.size(); i++)
    {
        unordered_map<string, int> * cnt = subCnt[i];
        for (auto item = cnt->begin(); item != cnt->end(); item++)
        {
            auto [str, cnt] = *item;
            int count = cnt;

            for (size_t nextIndex = i + 1; nextIndex < subCnt.size(); nextIndex++)
            {
                unordered_map<string, int> * nextCnt = subCnt[nextIndex];
                if (nextCnt->contains(str))
                {
                    count += (*nextCnt)[str];
                    nextCnt->erase(str);
                }
            }
            result.push_back(str + ' ' + to_string(count));
        }
    }

    ofstream outFile(TMP_FILE_DIR + to_string(fileIndex));

    // sort word count result by word
    sort(result.begin(), result.end(), greater<>());
    ostream_iterator<string> output_iterator(outFile, "\n");
    copy(result.begin(), result.end(), output_iterator);


    for (auto sLine : lines)
    {
        sLine->clear();
        delete sLine;
    }
}

void mergePartialTopN(OrderedProvider<UrlCnt> * provider, vector<UrlCnt> & result)
{
    priority_queue<tuple<string, int, int>> heap;
    for (size_t index = 0; index < provider->size(); index++)
    {
        if (!provider->empty(index))
        {
            auto [str, cnt] = provider->pop(index);
            heap.push({str, cnt, index});
        }
    }

    vector<int> tmpIndex;

    Heap topN;
    while (!heap.empty())
    {
        auto [min, cnt, index] = heap.top();
        heap.pop();

        int count = cnt;
        tmpIndex.push_back(index);
        while (!heap.empty())
        {
            auto [tmpMin, cnt, index] = heap.top();
            if (tmpMin == min)
            {
                tmpIndex.push_back(index);
                count += cnt;
                heap.pop();
            }
            else
            {
                break;
            }
        }

        for (auto index : tmpIndex)
        {
            if (!provider->empty(index))
            {
                auto [newStr, newCnt] = provider->pop(index);
                if (newStr != "")
                {
                    heap.push({newStr, newCnt, index});
                }
            }
        }

        topN.push({min, count});
        tmpIndex.clear();
        if (topN.size() > N)
        {
            topN.pop();
        }
    }


    while (!topN.empty())
    {
        result.push_back(topN.top());
        topN.pop();
    }
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        cout << "Please provide file.\neg. main /tmp/urls\n";
        return 1;
    }

    char * filePath = argv[1];
    ifstream inFile(filePath);

    const auto processor_count = std::thread::hardware_concurrency();
    // int processor_count = 1;

    int subFileCount = 0;

    vector<vector<string> *> lines;
    vector<string> * subLines = new vector<string>();
    size_t subSize = MEM_SIZE / processor_count;

    // read file and create partial topn
    string line;
    size_t size = 0;
    while (getline(inFile, line))
    {
        if ((size + line.size()) < subSize)
        {
            subLines->push_back(line);
            size += line.size();
        }
        else
        {
            lines.push_back(subLines);
            subLines = new vector<string>();
            subLines->push_back(line);
            size = line.size();
        }

        if (lines.size() == processor_count)
        {
            prtialTopN(lines, subFileCount);
            subFileCount++;
            lines.clear();
        }
    }

    if (subLines->size() > 0)
    {
        lines.push_back(subLines);
    }

    if (lines.size() > 0)
    {
        prtialTopN(lines, subFileCount);
        subFileCount++;
        lines.clear();
    }

    vector<ifstream *> files = vector<ifstream *>(subFileCount);
    for (int index = 0; index < subFileCount; index++)
    {
        files[index] = new ifstream(TMP_FILE_DIR + to_string(index));
    }

    // merge partial top n
    OrderedProvider<UrlCnt> * orderProvider = new FileOrderedProvider(move(files));
    vector<UrlCnt> result;
    result.clear();
    mergePartialTopN(orderProvider, result);


    // print result;
    cout << "result:\n";
    for (vector<UrlCnt>::reverse_iterator i = result.rbegin(); i != result.rend(); ++i)
    {
        auto [str, count] = *i;
        cout << str << ":" << count << endl;
    }

    return 0;
}
