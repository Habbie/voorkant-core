#include "main.hpp"

#include "ext/argparse/include/argparse/argparse.hpp"

#include <iostream>
#include <string>
#include <unistd.h>

#include "Backend.hpp"
#include "Observer.hpp"
#include "src/logger.hpp"

using std::string;

using std::cerr;
using std::cout;
using std::endl;

class SimpleObserver : public IObserver
{
public:
  ~SimpleObserver()
  {
    haentity->detach((IObserver*)this);
  }
  SimpleObserver(std::shared_ptr<HAEntity> _entity)
  {
    haentity = _entity;
    haentity->attach((IObserver*)this);
  }
  void update() override
  {
    g_log << Logger::Debug << "Received uiupdate for " << haentity->name << ":" << std::endl;
    g_log << Logger::Debug << haentity->getInfo() << std::endl;
  }

private:
  std::shared_ptr<HAEntity> haentity;
};

// static bool uithread_refresh_print_updates = false;

void uithread(int _argc, char* _argv[])
{
  argparse::ArgumentParser program("voorkant-cli", getVersion());
  argparse::ArgumentParser version_command("version");
  program.add_subparser(version_command);
  argparse::ArgumentParser subscribe_command("subscribe");
  subscribe_command.add_argument("domain")
    .help("specific a HA domain"); // maybe .remaining() so you can subscribe multiple?

  program.add_subparser(subscribe_command);

  argparse::ArgumentParser token_command("ha-get-token");
  token_command.add_argument("name").help("Name of the token").default_value("voorkant");
  program.add_subparser(token_command);

  argparse::ArgumentParser list_entities_command("list-entities");
  program.add_subparser(list_entities_command);

  /* usage example for dump-command with data:
   * build/voorkant-cli dump-command call_service '{"domain":"light","service":"toggle","target":{"entity_id":"light.bed_light"}}'
   */
  argparse::ArgumentParser dump_command("dump-command");
  dump_command.add_argument("command").help("the command to execute");
  dump_command.add_argument("data").help("optional data to pass with the command").default_value("{}");

  program.add_subparser(dump_command);

  try {
    program.parse_args(_argc, _argv);
  }
  catch (const std::runtime_error& err) {
    cerr << err.what() << endl;
    cerr << program;
    return;
  }

  if (program.is_subcommand_used(version_command)) {
    cout << getVersion() << endl;
    return;
  }
  else if (program.is_subcommand_used(subscribe_command)) {
    // FIXME: now actually use the argument
    string domain = subscribe_command.get<string>("domain");
    if (!domain.empty()) {
      cout << "should subscribe to " << subscribe_command.get<string>("domain") << endl;
    }
    else {
      cerr << "No domain provided!" << endl;
      return;
    }

    HABackend::getInstance().start();

    auto haentities = HABackend::getInstance().getEntitiesByDomain(domain);
    std::vector<std::unique_ptr<SimpleObserver>> observers;
    for (const auto& haentity : haentities) {
      std::cerr << "Monitoring entity: " << haentity->name << std::endl;
      std::unique_ptr<SimpleObserver> observer = std::make_unique<SimpleObserver>(haentity);
      observers.push_back(std::move(observer));
    }

    while (true) {
      usleep(10 * 1000 * 1000);
    }
  }
  else if (program.is_subcommand_used(token_command)) {
    string token = HABackend::getInstance().createLongToken(token_command.get<string>("name"));
    cout << token << endl;
  }
  else if (program.is_subcommand_used(list_entities_command)) {
    HABackend::getInstance().start();
    for (const auto& [entityname, entity] : HABackend::getInstance().getEntities()) {
      cout << entityname << endl;
    }
  }
  else if (program.is_subcommand_used(dump_command)) {
    HABackend::getInstance().start();
    json data = json::parse(dump_command.get<string>("data"));
    json res = HABackend::getInstance().doCommand(dump_command.get<string>("command"), data);
    cout << res.dump(2) << endl;
  }
  else {
    cerr << "no command given" << endl;
  }
}
