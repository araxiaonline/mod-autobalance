DROP TABLE IF EXISTS group_difficulty;
CREATE TABLE group_difficulty
(
    guid                 int unsigned     default '0' not null primary key,
    difficulty               tinyint unsigned default '0' not null,
    PRIMARY KEY (`guid`)
)
