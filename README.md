# BeastMaster NPC


## Description

This module allows each class (even Warlocks and Death Knights) to use hunter pets by adding a special NPC. This NPC provides the following options:
- teach the necessary hunter skills
- provide different pets (even exotic) which can be customized in the config file
- food for the pets
- stables (only for hunters)

The module can be configured to be restricted to hunters only. In this mode it can serve as a source for rare pets specified in the config file.


## How to use ingame

As GM:
- add NPC permanently:
 ```
 .npc add 601026
 ```
- add NPC temporarily:
 ```
 .npc add temp 601026
 ```


## Installation

Clone Git repository:

```
cd <ACdir>
git clone https://gitlab.com/opfesoft/mod-npc-beastmaster.git modules/mod-npc-beastmaster
```

Import SQL automatically:
```
cd <ACdir>
bash apps/db_assembler/db_assembler.sh
```
choose 8)

Import SQL manually:
```
cd <ACdir>
bash apps/db_assembler/db_assembler.sh
```
choose 4)
```
cd <ACdir>
mysql -P <DBport> -u <DPuser> --password=<DBpassword> world <local/sql/world_custom.sql
```


## Edit module configuration (optional)

If you need to change the module configuration, go to your server configuration folder (where your `worldserver` or `worldserver.exe` is), copy `npc_beastmaster.conf.dist` to `npc_beastmaster.conf` and edit that new file.


## Credits

* Stoabrogga: further development
* [Talamortis](https://github.com/talamortis): further development
* [BarbzYHOOL](https://github.com/barbzyhool): support
* [StygianTheBest](http://stygianthebest.github.io): original author (this module is based on v2017.09.03)


## License
This code and content is released under the [GNU AGPL v3](LICENSE.md).
