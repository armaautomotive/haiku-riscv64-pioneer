/* SPDX-License-Identifier: MIT */
#ifndef ASSISTANT_P100A_QWEN27_H
#define ASSISTANT_P100A_QWEN27_H
#include <string>
#include <functional>
class AssistantP100AQwen27 {
 struct Data; Data* fData; bool fFaulted;
public:
 AssistantP100AQwen27();
 ~AssistantP100AQwen27();
 static bool Available(); // executable presence ONLY; worker later checks CPU metadata, no BEGIN
 bool Load(const char* path,unsigned threads,std::string& error,const std::function<void(float,const char*)>& progress);
 bool SetThreads(unsigned threads,std::string& error); // idle only; retains model/context/card weights
 bool Complete(const std::string& question,int tokens,std::string& answer,std::string& error,
  const std::function<bool()>& cancelled,const std::function<void(const std::string&,const char*)>& progress,
  const std::function<void(float,const char*)>& memoryProgress);
 bool Unload(std::string& error);
 bool Faulted() const { return fFaulted; }
private:
 AssistantP100AQwen27(const AssistantP100AQwen27&);
 AssistantP100AQwen27& operator=(const AssistantP100AQwen27&);
};
#endif
