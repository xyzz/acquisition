/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "testdata.h"

#include <string>

const std::string kItem1 = 
   "{\"verified\":false,\"w\":2,\"h\":2,\"icon\":\"http://webcdn.pathofexile.com/image/Art/2DItems/Armours/Helmets/Helm"
   "etStrDex10.png?scale=1&w=2&h=2&v=0a540f285248cdb64d4607186e348e3d3\",\"support\":true,\"league\":\"Rampage\",\"sock"
   "ets\":[{\"group\":0,\"attr\":\"D\"}],\"name\":\"Demon Ward\",\"typeLine\":\"Nightmare Bascinet\",\"identified\":tru"
   "e,\"corrupted\":false,\"properties\":[{\"name\":\"Quality\",\"values\":[[\"+20%\",1]],\"displayMode\":0},{\"name\":"
   "\"Armour\",\"values\":[[\"310\",1]],\"displayMode\":0},{\"name\":\"Evasion Rating\",\"values\":[[\"447\",1]],\"disp"
   "layMode\":0}],\"requirements\":[{\"name\":\"Level\",\"values\":[[\"67\",0]],\"displayMode\":0},{\"name\":\"Str\",\""
   "values\":[[\"62\",0]],\"displayMode\":1},{\"name\":\"Dex\",\"values\":[[\"85\",0]],\"displayMode\":1}],\"explicitMo"
   "ds\":[\"+93 to Accuracy Rating\",\"100% increased Armour and Evasion\",\"+24% to Fire Resistance\",\"+32% to Lightn"
   "ing Resistance\",\"15% increased Block and Stun Recovery\"],\"frameType\":2,\"x\":0,\"y\":0,\"inventoryId\": \"Helm"
   "\",\"socketedItems\":[], \"_type\": 0, \"_tab_label\": \"x\", \"_tab\": 0, \"_x\": 0, \"_y\": 0}";

const std::string kCategoriesItemCard =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":0,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Divi"
   "nation\\/InventoryIcon.png?scale=1&scaleIndex=0&stackSize=1&w=1&h=1&v=a8ae131b97fad3c64de0e6d9f250d743\",\"league\""
   ":\"Standard\",\"id\":\"9aae0ab91a6fd7ceef94538b74a5b0cc770a966f3089295cf0a45bc0e48bad84\",\"name\":\"\",\"typeLine"
   "\":\"The King\'s Heart\",\"identified\":true,\"properties\":[{\"name\":\"Stack Size\",\"values\":[[\"1\\/8\",0]],\""
   "displayMode\":0}],\"explicitMods\":[\"<uniqueitem>{Kaom\'s Heart}\"],\"flavourText\":[\"<size:31>{500 times Kaom\'s"
   " axe fell, 500 times Kaom\'s Heart splintered. Finally, all that remained was a terrible, heartless Fury.}\"],\"fra"
   "meType\":6,\"stackSize\":1,\"maxStackSize\":8,\"artFilename\":\"TheKingsHeart\",\"category\":{\"cards\":[]},\"x\":8"
   ",\"y\":11,\"inventoryId\":\"Stash3\"}";

const std::string kCategoriesItemBelt =
   "{\"verified\":false,\"w\":2,\"h\":1,\"ilvl\":80,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Bel"
   "ts\\/AbyssBelt.png?scale=1&scaleIndex=0&w=2&h=1&v=cd7c25de181e6a77812020eb6e867971\",\"league\":\"Standard\",\"id\""
   ":\"7aa1eaefc73c5fd0321c201675d17d3a1c9ef7ba2748b903cb8d4171a9a70637\",\"sockets\":[{\"group\":0,\"attr\":false,\"sC"
   "olour\":\"A\"}],\"name\":\"Ambush Cord\",\"typeLine\":\"Stygian Vise\",\"identified\":true,\"requirements\":[{\"nam"
   "e\":\"Level\",\"values\":[[\"64\",0]],\"displayMode\":0}],\"implicitMods\":[\"Has 1 Abyssal Socket\"],\"explicitMod"
   "s\":[\"+49 to maximum Energy Shield\",\"18% reduced Flask Charges used\",\"18% increased Elemental Damage with Atta"
   "ck Skills\"],\"frameType\":2,\"category\":{\"accessories\":[\"belt\"]},\"x\":0,\"y\":10,\"inventoryId\":\"Stash21\""
   ",\"socketedItems\":[]}";
const std::string kItemBeltPOB =
R"(Ambush Cord
Stygian Vise
Sockets: A
Implicits: 1
Has 1 Abyssal Socket
+49 to maximum Energy Shield
18% reduced Flask Charges used
18% increased Elemental Damage with Attack Skills)";

const std::string kCategoriesItemEssence =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":0,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Curr"
   "ency\\/Essence\\/Wrath5.png?scale=1&scaleIndex=3&stackSize=4&w=1&h=1&v=47288253244910c2da4f0bdda069b832\",\"league"
   "\":\"Standard\",\"id\":\"ec18c882e5438ed72a71337f3dfb9a3acf0a3afccc068acbdcf18adf07784f02\",\"name\":\"\",\"typeLin"
   "e\":\"Screaming Essence of Wrath\",\"identified\":true,\"properties\":[{\"name\":\"Stack Size\",\"values\":[[\"4\\/"
   "9\",0]],\"displayMode\":0}],\"explicitMods\":[\"Upgrades a normal item to rare with one guaranteed property\",\"\","
   "\"Two Handed Melee Weapon: Adds (5-16) to (200-211) Lightning Damage\",\"Other Weapon: Adds (4-11) to (133-140) Lig"
   "htning Damage\",\"Armour: (36-41)% to Lightning Resistance\",\"Quiver: (36-41)% to Lightning Resistance\",\"Belt: ("
   "36-41)% to Lightning Resistance\",\"Other Jewellery: (23-26)% increased Lightning Damage\"],\"descrText\":\"Right c"
   "lick this item then left click a normal item to apply it.\",\"frameType\":5,\"stackSize\":4,\"maxStackSize\":5000,"
   "\"category\":{\"currency\":[]},\"x\":44,\"y\":0,\"inventoryId\":\"Stash1\"}";

const std::string kCategoriesItemVaalGem =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":0,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Gems"
   "\\/VaalGems\\/VaalArc.png?scale=1&scaleIndex=3&w=1&h=1&v=b4f32328e279496ebb227521e8dce679\",\"support\":false,\"id"
   "\":\"d1014d4e4e6ecaa53805cfa274069071d4b3d26aa0f8a9ac1680cb77f5ed0f47\",\"name\":\"\",\"typeLine\":\"Vaal Arc\",\"i"
   "dentified\":true,\"corrupted\":true,\"properties\":[{\"name\":\"Vaal, Spell, Chaining, Lightning, Duration\",\"valu"
   "es\":[],\"displayMode\":0},{\"name\":\"Level\",\"values\":[[\"14\",0]],\"displayMode\":0,\"type\":5},{\"name\":\"Ma"
   "na Cost\",\"values\":[[\"22\",0]],\"displayMode\":0},{\"name\":\"Cast Time\",\"values\":[[\"0.80 sec\",0]],\"displa"
   "yMode\":0},{\"name\":\"Critical Strike Chance\",\"values\":[[\"5.00%\",0]],\"displayMode\":0},{\"name\":\"Effective"
   "ness of Added Damage\",\"values\":[[\"90%\",0]],\"displayMode\":0}],\"additionalProperties\":[{\"name\":\"Experienc"
   "e\",\"values\":[[\"4312826\\/5099360\",0]],\"displayMode\":2,\"progress\":0.845758318901062,\"type\":20}],\"require"
   "ments\":[{\"name\":\"Level\",\"values\":[[\"56\",0]],\"displayMode\":0},{\"name\":\"Int\",\"values\":[[\"125\",0]],"
   "\"displayMode\":1}],\"secDescrText\":\"An arc of lightning stretches from the caster to a targeted enemy and chains"
   " on to other nearby enemies. Each time the main beam chains it will also chain to a second enemy, but that secondar"
   "y arc cannot chain further.\",\"explicitMods\":[\"Deals 80 to 456 Lightning Damage\",\"Chains +6 Times\",\"10% chan"
   "ce to Shock enemies\",\"15% more Damage for each remaining Chain\",\"23% increased Effect of Shock\"],\"descrText\""
   ":\"Place into an item socket of the right colour to gain this skill. Right click to remove from a socket.\",\"frame"
   "Type\":4,\"category\":{\"gems\":[\"activegem\"]},\"socket\":1,\"colour\":\"I\",\"vaal\":{\"baseTypeName\":\"Arc\","
   "\"properties\":[{\"name\":\"Souls Per Use\",\"values\":[[\"25\",0]],\"displayMode\":0},{\"name\":\"Can Store %0 Use"
   "\",\"values\":[[\"1\",0]],\"displayMode\":3},{\"name\":\"Soul Gain Prevention\",\"values\":[[\"6 sec\",0]],\"displa"
   "yMode\":0},{\"name\":\"Cast Time\",\"values\":[[\"0.80 sec\",0]],\"displayMode\":0},{\"name\":\"Critical Strike Cha"
   "nce\",\"values\":[[\"5.00%\",0]],\"displayMode\":0},{\"name\":\"Effectiveness of Added Damage\",\"values\":[[\"180%"
   "\",0]],\"displayMode\":0}],\"explicitMods\":[\"Deals 483 to 806 Lightning Damage\",\"Base duration is 4.00 seconds"
   "\",\"Chains +8 Times\",\"Modifiers to Buff Duration also apply to this Skill\'s Soul Gain Prevention\",\"100% incre"
   "ased Shock Duration on enemies\",\"100% chance to Shock enemies\",\"15% more Damage for each remaining Chain\",\"10"
   "0% increased Effect of Shock\"],\"secDescrText\":\"A shocking arc of lightning stretches from the caster to a targe"
   "ted enemy and chains to other nearby enemies. Each time the beam chains it will also chain simultaneously to a seco"
   "nd enemy, but no enemy can be hit twice by the beams. Also grants a buff making you lucky when damaging enemies wit"
   "h Arc for a short duration.\"}}";

const std::string kCategoriesItemSupportGem =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":0,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Gems"
   "\\/Support\\/IncreasedQuantity.png?scale=1&scaleIndex=0&w=1&h=1&v=b07c717ecf613d726e9edcbc0d66ff93\",\"support\":tr"
   "ue,\"league\":\"Hardcore\",\"id\":\"01726cfdcd593fafa3137f11274505c8678fc982b93146363b424165d1de1fe8\",\"name\":\""
   "\",\"typeLine\":\"Item Quantity Support\",\"identified\":true,\"properties\":[{\"name\":\"Support\",\"values\":[],"
   "\"displayMode\":0},{\"name\":\"Level\",\"values\":[[\"3\",0]],\"displayMode\":0,\"type\":5}],\"additionalProperties"
   "\":[{\"name\":\"Experience\",\"values\":[[\"210204\\/254061\",0]],\"displayMode\":2,\"progress\":0.827376127243042,"
   "\"type\":20}],\"requirements\":[{\"name\":\"Level\",\"values\":[[\"30\",0]],\"displayMode\":0},{\"name\":\"Str\",\""
   "values\":[[\"51\",0]],\"displayMode\":1}],\"secDescrText\":\"Supports any skill that can kill enemies.\",\"explicit"
   "Mods\":[\"19% increased Quantity of Items Dropped by Enemies Slain from Supported Skills\"],\"descrText\":\"This is"
   " a Support Gem. It does not grant a bonus to your character, but to skills in sockets connected to it. Place into a"
   "n item socket connected to a socket containing the Active Skill Gem you wish to augment. Right click to remove from"
   " a socket.\",\"frameType\":4,\"category\":{\"gems\":[\"supportgem\"]},\"x\":11,\"y\":4,\"inventoryId\":\"Stash5\"}";

const std::string kCategoriesItemBow =
   "{\"verified\":false,\"w\":2,\"h\":4,\"ilvl\":74,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Wea"
   "pons\\/TwoHandWeapons\\/Bows\\/BowOfTheCouncil.png?scale=1&scaleIndex=0&w=2&h=4&v=ecb677fbcbc747a46df9fa5e1e919952"
   "\",\"league\":\"Standard\",\"id\":\"b65efbb946441039d6a0bc0d224d01f732ec15828fcae511d6bde5a7d5a41622\",\"sockets\":"
   "[{\"group\":0,\"attr\":\"D\",\"sColour\":\"G\"},{\"group\":1,\"attr\":\"D\",\"sColour\":\"G\"},{\"group\":1,\"attr"
   "\":\"I\",\"sColour\":\"B\"},{\"group\":1,\"attr\":\"I\",\"sColour\":\"B\"},{\"group\":1,\"attr\":\"D\",\"sColour\":"
   "\"G\"},{\"group\":1,\"attr\":\"D\",\"sColour\":\"G\"}],\"name\":\"Reach of the Council\",\"typeLine\":\"Spine Bow\""
   ",\"identified\":true,\"properties\":[{\"name\":\"Bow\",\"values\":[],\"displayMode\":0},{\"name\":\"Quality\",\"val"
   "ues\":[[\"+20%\",1]],\"displayMode\":0,\"type\":6},{\"name\":\"Physical Damage\",\"values\":[[\"84-249\",1]],\"disp"
   "layMode\":0,\"type\":9},{\"name\":\"Critical Strike Chance\",\"values\":[[\"6.00%\",0]],\"displayMode\":0,\"type\":"
   "12},{\"name\":\"Attacks per Second\",\"values\":[[\"1.49\",1]],\"displayMode\":0,\"type\":13}],\"requirements\":[{"
   "\"name\":\"Level\",\"values\":[[\"68\",0]],\"displayMode\":0},{\"name\":\"Dex\",\"values\":[[\"212\",0]],\"displayM"
   "ode\":1},{\"name\":\"Int\",\"values\":[[\"108\",0]],\"displayMode\":1}],\"explicitMods\":[\"44% increased Physical "
   "Damage\",\"Adds 24 to 72 Physical Damage\",\"10% increased Attack Speed\",\"Bow Attacks fire 4 additional Arrows\","
   "\"20% reduced Projectile Speed\"],\"flavourText\":[\"We stand together. We strike together.\"],\"frameType\":3,\"ca"
   "tegory\":{\"weapons\":[\"bow\"]},\"x\":0,\"y\":0,\"inventoryId\":\"Weapon2\",\"socketedItems\":[]}";
const std::string kItemBowPOB =
R"(Reach of the Council
Spine Bow
Sockets: G G-B-B-G-G
Implicits: 0
44% increased Physical Damage
Adds 24 to 72 Physical Damage
10% increased Attack Speed
Bow Attacks fire 4 additional Arrows
20% reduced Projectile Speed)";

const std::string kCategoriesItemClaw =
   "{\"verified\":false,\"w\":2,\"h\":2,\"ilvl\":70,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Wea"
   "pons\\/OneHandWeapons\\/Claws\\/Claw5Unique2.png?scale=1&scaleIndex=0&w=2&h=2&v=1b50572643c65320c8726d57a01d5be4\","
   "\"league\":\"Standard\",\"id\":\"f66fa2861857808c403ce958f1c38fcd7ed1bc8eda4ef5bf3f84f4f3edb99b09\",\"sockets\":[{"
   "\"group\":0,\"attr\":\"D\",\"sColour\":\"G\"},{\"group\":0,\"attr\":\"D\",\"sColour\":\"G\"}],\"name\":\"Essentia S"
   "anguis\",\"typeLine\":\"Eye Gouger\",\"identified\":true,\"properties\":[{\"name\":\"Claw\",\"values\":[],\"display"
   "Mode\":0},{\"name\":\"Physical Damage\",\"values\":[[\"56-147\",1]],\"displayMode\":0,\"type\":9},{\"name\":\"Eleme"
   "ntal Damage\",\"values\":[[\"1-50\",6]],\"displayMode\":0,\"type\":10},{\"name\":\"Critical Strike Chance\",\"value"
   "s\":[[\"6.30%\",0]],\"displayMode\":0,\"type\":12},{\"name\":\"Attacks per Second\",\"values\":[[\"1.83\",1]],\"dis"
   "playMode\":0,\"type\":13},{\"name\":\"Weapon Range\",\"values\":[[\"9\",0]],\"displayMode\":0,\"type\":14}],\"requi"
   "rements\":[{\"name\":\"Level\",\"values\":[[\"64\",0]],\"displayMode\":0},{\"name\":\"Dex\",\"values\":[[\"113\",0]"
   "],\"displayMode\":1},{\"name\":\"Int\",\"values\":[[\"113\",0]],\"displayMode\":1}],\"implicitMods\":[\"0.6% of Phy"
   "sical Attack Damage Leeched as Life\"],\"explicitMods\":[\"+10% Chance to Block Attack Damage while Dual Wielding C"
   "laws\",\"116% increased Physical Damage\",\"Adds 1 to 50 Lightning Damage\",\"22% increased Attack Speed\",\"+32 to"
   " maximum Energy Shield\",\"Life Leech is applied to Energy Shield instead\"],\"flavourText\":[\"The darkest clouds "
   "clashed and coupled,\\r\",\"giving birth to four lightning children of hate.\"],\"frameType\":3,\"category\":{\"wea"
   "pons\":[\"claw\"]},\"x\":6,\"y\":6,\"inventoryId\":\"Stash4\",\"socketedItems\":[]}";
const std::string kItemClawPOB =
R"(Essentia Sanguis
Eye Gouger
Sockets: G-G
Implicits: 1
0.6% of Physical Attack Damage Leeched as Life
+10% Chance to Block Attack Damage while Dual Wielding Claws
116% increased Physical Damage
Adds 1 to 50 Lightning Damage
22% increased Attack Speed
+32 to maximum Energy Shield
Life Leech is applied to Energy Shield instead)";

const std::string kCategoriesItemFragment =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":0,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Maps"
   "\\/Vaal01.png?scale=1&scaleIndex=0&w=1&h=1&v=3b21ce0cd4c0b9e8cf5db6257daf831a\",\"league\":\"Standard\",\"id\":\"57"
   "5e571ccbea9ed070a1dd473794252a2ffe2aee6009e29050d6fad16c98a0af\",\"name\":\"\",\"typeLine\":\"Sacrifice at Midnight"
   "\",\"identified\":true,\"descrText\":\"Can be used in the Templar Laboratory or a personal Map Device.\",\"flavourT"
   "ext\":[\"Look to our Queen, for she will lead us into the light.\"],\"frameType\":0,\"category\":{\"maps\":[]},\"x"
   "\":3,\"y\":3,\"inventoryId\":\"MainInventory\"}";

const std::string kCategoriesItemWarMap =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":83,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Map"
   "s\\/Atlas2Maps\\/New\\/SunkenCity.png?scale=1&scaleIndex=0&w=1&h=1&mn=1&mt=15&v=da5c60832e40b287dc576c7f806448ce\","
   "\"league\":\"Standard\",\"id\":\"4ffac7327fa9ecf0996c9c7e456f9d53fbb2613f7bdbfd8f33faa2a36761355e\",\"name\":\"\","
   "\"typeLine\":\"Sunken City Map\",\"identified\":true,\"properties\":[{\"name\":\"Map Tier\",\"values\":[[\"15\",0]]"
   ",\"displayMode\":0,\"type\":1}],\"descrText\":\"Travel to this Map by using it in the Templar Laboratory or a perso"
   "nal Map Device. Maps can only be used once.\",\"frameType\":0,\"category\":{\"maps\":[]},\"x\":9,\"y\":11,\"invento"
   "ryId\":\"Stash2\"}";

const std::string kCategoriesItemUniqueMap =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":74,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Map"
   "s\\/HallOfGrandmasters.png?scale=1&scaleIndex=0&w=1&h=1&v=7b8e637fc9b6f7da6631cb8c48ce6af7\",\"league\":\"Standard"
   "\",\"id\":\"006611ccbc68137c27db8ea1fd735af15ba5c1828691bf4ad1fde2e43e2f1883\",\"name\":\"Hall of Grandmasters\",\""
   "typeLine\":\"Promenade Map\",\"identified\":true,\"properties\":[{\"name\":\"Map Tier\",\"values\":[[\"4\",0]],\"di"
   "splayMode\":0,\"type\":1}],\"explicitMods\":[\"Contains the Immortalised Grandmasters\\nPvP damage scaling in effec"
   "t\"],\"descrText\":\"Travel to this Map by using it in the Templar Laboratory or a personal Map Device. Maps can on"
   "ly be used once.\",\"flavourText\":[\"The grandest and greatest ever to fight,\\r\",\"Divine the champions stand ta"
   "ll.\\r\",\"But match their power, best their might,\\r\",\"And even the immortal may fall.\"],\"frameType\":3,\"cat"
   "egory\":{\"maps\":[]},\"x\":5,\"y\":2,\"inventoryId\":\"MainInventory\"}";

const std::string kCategoriesItemBreachstone =
   "{\"verified\":false,\"w\":1,\"h\":1,\"ilvl\":0,\"icon\":\"https:\\/\\/web.poecdn.com\\/image\\/Art\\/2DItems\\/Curr"
   "ency\\/Breach\\/BreachFragmentsLightning.png?scale=1&scaleIndex=0&w=1&h=1&v=01c1ec0220d90a59ebdbb1847a915710\",\"le"
   "ague\":\"Standard\",\"id\":\"aeebf65ba96865632773520742e8e901b8d7c064e93de927fa57296e1659e749\",\"name\":\"\",\"typ"
   "eLine\":\"Esh\'s Breachstone\",\"identified\":true,\"descrText\":\"Travel to Esh\'s Domain by using this item in th"
   "e Templar Laboratory or a personal Map Device. Can only be used once.\",\"frameType\":0,\"category\":{\"maps\":[]},"
   "\"x\":11,\"y\":1,\"inventoryId\":\"Stash58\"}";

const std::string kSocketedItem =
   "{\"verified\":false,\"w\":2,\"h\":2,\"icon\":\"http://webcdn.pathofexile.com/image/Art/2DItems/Armours/Helmets/Helm"
   "etStrDex10.png?scale=1&w=2&h=2&v=0a540f285248cdb64d4607186e348e3d3\",\"support\":true,\"league\":\"Rampage\",\"sock"
   "ets\":[{\"group\":0,\"attr\":\"D\"}],\"name\":\"Demon Ward\",\"typeLine\":\"Nightmare Bascinet\",\"identified\":tru"
   "e,\"corrupted\":false,\"properties\":[{\"name\":\"Quality\",\"values\":[[\"+20%\",1]],\"displayMode\":0},{\"name\":"
   "\"Armour\",\"values\":[[\"310\",1]],\"displayMode\":0},{\"name\":\"Evasion Rating\",\"values\":[[\"447\",1]],\"disp"
   "layMode\":0}],\"requirements\":[{\"name\":\"Level\",\"values\":[[\"67\",0]],\"displayMode\":0},{\"name\":\"Str\",\""
   "values\":[[\"62\",0]],\"displayMode\":1},{\"name\":\"Dex\",\"values\":[[\"85\",0]],\"displayMode\":1}],\"explicitMo"
   "ds\":[\"+93 to Accuracy Rating\",\"100% increased Armour and Evasion\",\"+24% to Fire Resistance\",\"+32% to Lightn"
   "ing Resistance\",\"15% increased Block and Stun Recovery\"],\"frameType\":2,\"x\":0,\"y\":0,\"inventoryId\": \"\", "
   "\"socketedItems\":[], \"_socketed\":true, \"_type\": 0, \"_tab_label\": \"label\", \"_tab\": 0, \"_x\": 0, \"_y\": "
   "0}";
