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
cd <SolDir>
git clone https://gitlab.com/opfesoft/mod-npc-beastmaster.git modules/mod-npc-beastmaster
```

Import SQL:
```
cd <SolDir>
bash bash/db_assembler.sh 4
mysql -P <DBport> -u <DPuser> --password=<DBpassword> world <local/sql/world_custom.sql
```

Without DB Assembler:
```
cd <SolDir>
mysql -P <DBport> -u <DPuser> --password=<DBpassword> world <modules/mod-npc-beastmaster/data/sql/db-world/npc_beastmaster.sql
```


## Edit module configuration (optional)

If you need to change the module configuration, go to your server configuration folder (e.g. `sol-srv/etc`), copy `npc_beastmaster.conf.dist` to `npc_beastmaster.conf` and edit that new file.


## Credits

* [Stoabrogga](https://gitlab.com/Stoabrogga): further development
* [Talamortis](https://github.com/talamortis): further development
* [BarbzYHOOL](https://github.com/barbzyhool): support
* [StygianTheBest](https://stygianthebest.github.io): original author (this module is based on v2017.09.30)


## License

This code and content is released under the [GNU AGPL v3](LICENSE.md).
