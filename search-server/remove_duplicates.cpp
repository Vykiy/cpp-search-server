#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server){
    vector<int> remove_docs;
    set<set<string>> unique_docs_words;
    for (const int &document_id : search_server) {
        set<string> words;
        for(const auto [word, freq] : search_server.GetWordFrequencies(document_id)){
            words.insert(word);
        }
        if(unique_docs_words.find(words) == unique_docs_words.end()){
            unique_docs_words.insert(words);
        }else{
            remove_docs.emplace_back(document_id);
        }
        
    }
    for (const auto& id : remove_docs){
        std::cout << "Found duplicate document id "s << id << '\n';
        search_server.RemoveDocument(id);
    }
}