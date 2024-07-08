DROP TABLE IF EXISTS acore_characters.group_difficulty;
CREATE TABLE acore_characters.group_difficulty
(
    group_id                 int unsigned     default '0' not null primary key,
    difficulty               tinyint unsigned default '0' not null
)
