CREATE TABLE IF NOT EXISTS `mod_npc_beastmaster_custom_auras` (
  `PlayerGUIDLow` int(10) unsigned NOT NULL,
  `PetID` int(10) unsigned NOT NULL,
  `CustomAura` int(10) unsigned NOT NULL,
  INDEX (`PlayerGUIDLow`,`PetID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='mod-npc-beastmaster; used for custom auras';
