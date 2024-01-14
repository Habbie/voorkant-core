#include <iostream>
#include <string>
#include <thread>
#include "Backend.hpp"
#include "HAEntity.hpp"
#include "main.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::thread;

HABackend::HABackend(){};

bool HABackend::Connect(string url, string token)
{
  cerr << "[HABackend] Connecting to " << url << endl;

  wc = new WSConn(url);
  auto welcome = wc->recv();
  auto jwelcome = json::parse(welcome);

  json auth;
  auth["type"] = "auth";
  auth["access_token"] = token;
  wc->send(auth);
  json authresponse = json::parse(wc->recv());
  if (authresponse["type"] != "auth_ok") {
    cerr << "Authentication failed, please check your HA_API_TOKEN" << endl;
    return false;
  }

  return true;
};

bool HABackend::Start()
{
  if (wc == nullptr) {
    cerr << "We expect that you'd do a Connect() first." << endl;
    return false;
  }
  loaded = false;
  std::unique_lock<std::mutex> lck(load_lock);
  ha = std::thread(&HABackend::threadrunner, this);
  ha.detach();
  while (!loaded) {
    usleep(200);
    load_cv.wait(lck);
  };
  return true;
};

string HABackend::CreateLongToken(string name)
{
  json tokenrequest;
  tokenrequest["type"] = "auth/long_lived_access_token";
  tokenrequest["client_name"] = name;
  tokenrequest["lifespan"] = 365;

  wc->send(tokenrequest);

  // TODO: should probably using a ref to fill the response and use bool as return type
  //{"id":1,"type":"result","success":true, "result":"TOKEN_HERE"}
  string response = wc->recv();
  json jresponse = json::parse(response);
  if (jresponse["success"] == true) {
    return jresponse["result"];
  }
  cerr << "Failed to create token: " << response << endl;
  return "NO_TOKEN";
}

json HABackend::DoCommand(const string& command, const json& data)
{
  json request;
  request = data;
  request["type"] = command;
  wc->send(request);
  auto response = wc->recv();

  json jsonresponse = json::parse(response);
  if (jsonresponse["id"] != request["id"]) {
    throw std::runtime_error("Send out a command, but received something with a different ID.");
  }
  return jsonresponse;
}

void HABackend::threadrunner()
{
  json getdomains;
  getdomains["type"] = "get_services";
  wc->send(getdomains);

  auto msg = wc->recv();
  json getdomainjson = json::parse(msg);
  if (getdomainjson["id"] != getdomains["id"]) {
    throw std::runtime_error("Didn't receive response to getDomains while we expected it");
  }
  for (auto& [domain, services] : getdomainjson["result"].items()) {

    std::scoped_lock lk(domainslock);
    domains[domain] = std::make_shared<HADomain>(domain, services);
  }
  cerr << "We have " << domains.size() << "domains " << endl;

  json subscribe;
  subscribe["type"] = "subscribe_events";
  wc->send(subscribe);

  json getstates;
  getstates["type"] = "get_states";
  wc->send(getstates);

  while (true) {
    auto msg = wc->recv();

    // cout<<msg<<endl;
    json j = json::parse(msg);

    std::vector<std::string> whatchanged;
    {

      if (j["id"] == getstates["id"]) {
        std::scoped_lock lk(entitieslock);
        // response for initial getstates call
        for (auto evd : j["result"]) {
          auto entity_id = evd["entity_id"].get<std::string>();
          // FIXME: boost::split might be nice here, check if its header only?
          auto pos = entity_id.find(".");
          if (pos == std::string::npos) {
            throw std::runtime_error("entity ID [" + entity_id + "] contains no period, has no domain?");
          }

          auto domain = entity_id.substr(0, pos);

          // FIXME: we should check if the domain actually exists before just calling for it.
          entities[entity_id] = std::make_shared<HAEntity>(evd, domains[domain]);
          whatchanged.push_back(entity_id);
        }
        std::unique_lock<std::mutex> lck(load_lock);
        loaded = true;
        load_cv.notify_all();
      }
      else if (j["type"] == "event") {
        std::scoped_lock lk(entitieslock);
        //  something happened!
        auto event = j["event"];
        auto event_type = event["event_type"];
        auto evd = event["data"];
        auto entity_id = evd["entity_id"];
        auto old_state = evd["old_state"];
        auto new_state = evd["new_state"];

        if (event_type == "state_changed") {
          entities[entity_id]->update(new_state);
          whatchanged.push_back(entity_id);
        }
        else {
          cerr << "Event type received that we didn't expect: " << event_type << endl;
        }
      }
      else {
        cerr << "Received message we don't expect: " << j["type"] << endl;
        // not a message we were expecting
        continue;
      }
    }

    uithread_refresh(this, whatchanged);
  }
}

map<string, std::shared_ptr<HAEntity>> HABackend::GetEntities()
{
  return entities;
}

std::shared_ptr<HAEntity> HABackend::GetEntityByName(const std::string& name)
{
  std::scoped_lock lk(entitieslock);

  return entities.at(name);
}

void HABackend::WSConnSend(json& msg)
{
  wc->send(msg);
}
