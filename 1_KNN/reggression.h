#include"typedef.h"

namespace KNN_REGRESSION
{
    class sentence
    {
    private:
        array<float, type_of_emotion>emoPercent = { 0 };
        int wordCnt = 0;
        string str;
        map<string, float> termFreq;
        set<string>word;
        void load(const string& s) {
            auto pos_of_comma = s.find_first_of(',');
            str = s.substr(0, pos_of_comma);
            const auto& dataStr = s.substr(pos_of_comma + 1);
            stringstream ss(str);
            string tmpStr;
            while (ss >> tmpStr) {
                wordCnt++;
                word.insert(tmpStr);
                termFreq[tmpStr]++;
            }
            sscanf_s(dataStr.c_str(), "%f,%f,%f,%f,%f,%f", &emoPercent[0], &emoPercent[1], &emoPercent[2], &emoPercent[3], &emoPercent[4], &emoPercent[5]);
            for (auto& str : termFreq) {
                str.second /= wordCnt;
            }
        }
        
    public:
        sentence() = default;
        explicit sentence(const string& s) {
            load(s);
        }
        ~sentence() = default;
        string getStr()const {
            return str;
        }
        int getWordCnt()const {
            return wordCnt;
        }
        float getWordFreq(string s)const {
            return termFreq.at(s);
        }
        const set<string>& getWordSet() const {
            return word;
        }
        const map < string, float >& getTF()const {
            return termFreq;
        }
        float get_prob(emotion e) const {
            return emoPercent[(int)e];
        }
        float correlate(const array<float, type_of_emotion>& res) {
            float r = COR<type_of_emotion>(res, emoPercent);
            if (isnan(r))throw overflow_error("line 95: COR overflow!");
            return r;
        }
        array<float, type_of_emotion>getEmotion()const {
            return emoPercent;
        }
        friend bool operator<(const sentence& s1, const sentence& s2) { return s1.str < s2.str; }
        template<size_t N>
        float COR(const array<float, N>& X, const array<float, N>& Y) {
            float sum = accumulate(X.begin(), X.end(), 0.0);
            float Xmean = sum / N;
            sum = accumulate(Y.begin(), Y.end(), 0.0);
            float Ymean = sum / N;
            float cov = 0, squareOfSigmaX = 0, squareOfSigmaY = 0;
            for (int i = 0; i < N; i++)
            {
                cov += (X[i] - Xmean) * (Y[i] - Ymean);
                squareOfSigmaX += pow(X[i] - Xmean, 2);
                squareOfSigmaY += pow(Y[i] - Ymean, 2);
            }
            if (squareOfSigmaX < 0 || squareOfSigmaY < 0)throw overflow_error("line 45:sqrt of minus number!");
            return cov / sqrt(squareOfSigmaX * squareOfSigmaY);
        }
        static float CORvec(const vector<float>& X, const vector<float>& Y) {
            const auto N = X.size();
            float sum = accumulate(X.begin(), X.end(), 0.0);
            float Xmean = sum / N;
            sum = accumulate(Y.begin(), Y.end(), 0.0);
            float Ymean = sum / N;
            float cov = 0, squareOfSigmaX = 0, squareOfSigmaY = 0;
            for (int i = 0; i < N; i++)
            {
                cov += (X[i] - Xmean) * (Y[i] - Ymean);
                squareOfSigmaX += pow(X[i] - Xmean, 2);
                squareOfSigmaY += pow(Y[i] - Ymean, 2);
            }
            if (squareOfSigmaX < 0 || squareOfSigmaY < 0)throw overflow_error("line 45:sqrt of minus number!");
            return cov / sqrt(squareOfSigmaX * squareOfSigmaY);
        }
    };


    class dataSet
    {
    private:
        vector<sentence> data;
        //IDF
        vector < float > InvDocFreq;
        //how many sentence the word appeared in
        vector<string> word;
        vector<int> wordFreq;
        vector<vector<float>> TF_IDF;
        bool TF_IDF_inited = false;
        void data_init(ifstream& infile) {
            multiset<sentence>sentenceSet;
            map<string, int> wordFreqMap;
            string s;
            getline(infile, s);
            while (getline(infile, s))
            {
                sentence sent(s);
                sentenceSet.insert(sent);
                auto TF = sent.getTF();
                for (auto& t : TF) {
                    wordFreqMap[t.first]++;
                }
                sentenceCnt++;
            }
            wordCnt = wordFreqMap.size();
            word.resize(wordCnt);
            wordFreq.resize(wordCnt);
            data.resize(sentenceCnt);
            transform(wordFreqMap.begin(), wordFreqMap.end(), word.begin(), [](pair<string, int> p) { return p.first; });
            transform(wordFreqMap.begin(), wordFreqMap.end(), wordFreq.begin(), [](pair<string, int> p) { return p.second; });
            transform(sentenceSet.begin(), sentenceSet.end(), data.begin(), [](sentence s) { return s; });
            //此后data有序
        }
        void TF_IDF_init() {
            if (sentenceCnt == 0 || wordCnt == 0)
                throw invalid_argument("regression.h::line 135:Data hasn't been initialized!");
            TF_IDF = vector<vector<float>>(sentenceCnt, vector<float>(wordCnt, 0));
            string currTerm;
            for (int i = 0; i < sentenceCnt; i++)
            {
                for (int j = 0; j < wordCnt; j++)
                {
                    currTerm = word[j];
                    const auto& TF = data[i].getTF();
                    if (TF.find(currTerm) != TF.end())
                        TF_IDF.at(i).at(j) = TF.at(currTerm) * InvDocFreq[j];
                    else TF_IDF.at(i).at(j) = 0;
                }
            }
            TF_IDF_inited = true;
        }
        void load(ifstream& infile) {
            data_init(infile);
            InvDocFreq.resize(wordCnt);
            for (int i = 0; i < wordCnt; i++) {
                InvDocFreq[i] = log10((float)sentenceCnt / (1 + wordFreq[i]));
            }
            TF_IDF_init();
        }
    protected:
        vector<float> encode(const sentence& s) {
            vector<float> code(wordCnt, 0);//编码用于计算距离
            auto& wordset = s.getWordSet();
            int i;//找到句子的单词在word数组中的位置
            for (auto w : wordset) {
                auto pos = find(word.begin(), word.end(), w);
                if (pos != word.end()) {
                    i = pos - word.begin();
                    code[i] = s.getWordFreq(w) * InvDocFreq[i];
                }
            }

            return code;
        }
        vector<pair<sentence*, float>> distLP(const sentence& s, codeType st, float p = 1) {
            vector<float>code = encode(s);
            if (p <= 0)throw invalid_argument("line 185: p must be positive!");
            vector<pair<sentence*, float>> res(sentenceCnt);
            for (int j = 0; j < sentenceCnt; j++)
            {
                const auto& v1 = TF_IDF[j];
                res[j].first = &data[j];
                res[j].second = 0;
                float lhs, rhs;
                for (int k = 0; k < wordCnt; k++)
                {
                    if (st == codeType::one_hot) {
                        const auto& sent = data[j];
                        lhs = v1[k] == 0 ? 0 : sent.getWordFreq(word[k]) * sent.getWordCnt();
                        rhs = code[k] == 0 ? 0 : s.getWordCnt() * s.getWordFreq(word[k]);
                    }
                    else
                        lhs = v1[k], rhs = code[k];
                    if (lhs != 0 || rhs != 0) {
                        res[j].second += pow(abs(lhs - rhs), p);
                    }
                }
                res[j].second = pow(res[j].second, 1 / p);
            }
            return res;
        }
        vector<pair<sentence*, float>> distCosine(const sentence& s, codeType st) {
            vector<float>code = encode(s);
            vector<pair<sentence*, float>> res(sentenceCnt);
            float normlhs = 0, normrhs = 0, lhs, rhs;
            for (int j = 0; j < sentenceCnt; j++)
            {
                const auto& v1 = TF_IDF[j];
                res[j].first = &data[j];
                res[j].second = 0;
                normlhs = 0, normrhs = 0;
                for (int k = 0; k < wordCnt; k++)
                {
                    if (st == codeType::one_hot) {
                        const auto& sent = data[j];
                        lhs = v1[k] == 0 ? 0 : sent.getWordFreq(word[k]) * sent.getWordCnt();
                        rhs = code[k] == 0 ? 0 : s.getWordCnt() * s.getWordFreq(word[k]);
                    }
                    else
                        lhs = v1[k], rhs = code[k];
                    if (lhs != 0 || rhs != 0) {
                        res[j].second += lhs * rhs;
                        normlhs += lhs * lhs;
                        normrhs += rhs * rhs;
                    }
                }
                if (normrhs == 0) {
                    res[j].second = 0;
                }
                else
                    res[j].second = 1 - res[j].second / sqrt(normlhs * normrhs);
            }
            return res;
        }
    public:
        dataSet() = default;
        dataSet(ifstream& infile) {
            load(infile);
        }
        const vector<vector<float>>& getTF_IDF()const {
            return TF_IDF;
        }
        int sentenceCnt = 0;
        int wordCnt = 0;
        ~dataSet() = default;
    };

    class KNN :public dataSet
    {
    public:
        KNN() = default;
        KNN(ifstream& infile) :dataSet(infile), k(1), p(1) {}
        ~KNN() = default;
        KNN& setK(int _k) { k = _k; return *this; }
        KNN& setP(int _p) { p = _p; return *this; }
        KNN& setCodeType(codeType _ct) { ct = _ct; return *this; }
        array<float, type_of_emotion> classify(const sentence& target) {
            array<float, type_of_emotion>res;
            vector<pair<sentence*, float>> vec;
            float inv_dist_sum = 0;
            res.fill(0);
            if (p == 0) {
                vec = distCosine(target, ct);
            }
            else
                vec = distLP(target, ct, p);
            sort(vec.begin(), vec.end(), [](const pair<sentence*, float>& p1, const pair<sentence*, float>& p2) {return p1.second < p2.second; });
            bool perfect_match = (vec[0].second < 1e-4);
            if (perfect_match) {
                int zeroCnt = 0;
                for (int i = 0; i < k; i++)
                {
                    if (vec[i].second != 0) continue;
                    zeroCnt++;
                    for (emotion e = emotion::anger; e < emotion::unknown; e = emotion((int)e + 1))
                    {
                        res[(int)e] += vec[i].first->get_prob(e);
                    }
                }
                transform(res.begin(), res.end(), res.begin(), [zeroCnt](float f) {return f / zeroCnt; });
            }
            else {
                for (int i = 0; i < k; i++)
                {
                    if (vec[i].second == 0)throw overflow_error("dist_reverese_sum = inf!");
                    inv_dist_sum += 1 / vec[i].second;
                }
                for (emotion e = emotion::anger; e < emotion::unknown; e = emotion(int(e) + 1))
                {
                    for (int i = 0; i < k; i++)
                    {
                        res[(int)e] += (vec[i].first->get_prob(e) / vec[i].second);
                    }
                    res[(int)e] /= inv_dist_sum;
                }
            }
            return res;
        }

    private:
        int k = 1;
        int p = 1;
        codeType ct = codeType::one_hot;
    };
    template<size_t N>
    float fullCOR(const vector < array<float, N>>& v1, const vector < array<float, N>>& v2) {
        int Asize = v1.size();
        array<vector<float>, type_of_emotion>vec1, vec2;
        vec1.fill(vector<float>(Asize, 0));
        vec2.fill(vector<float>(Asize, 0));
        for (int i = 0; i < Asize; i++)
        {
            for (int j = 0; j < type_of_emotion; j++)
            {
                vec1[j][i] = v1[i][j];
                vec2[j][i] = v2[i][j];
            }
        }
        float res = 0;
        for (int i = 0; i < type_of_emotion; i++)
        {
            res+=sentence::CORvec(vec1[i], vec2[i]);
        }
        return  res/ type_of_emotion;
    }
    KNN train(int max_k = 1, int max_p = 0);
    KNN train1(int max_k = 1, int max_p = 0);
    void test(KNN& k);
}
