select entry, eliteFactor from creature_template where eliteFactor != 1;

// Regexp to transform SqlYog export:
// insert into `creature_template` \(`entry`, `eliteFactor`\) values\('{[^']+}','{[^']+}'\);
// update creature_template set eliteFactor = \2 where entry = \1;
