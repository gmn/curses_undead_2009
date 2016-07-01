#include "g_item.h"

//
itemDef_s itemDefsTable[] = { 
    // WEAPONS
    { ITEM_WEAPON, ITEM_TEMPLATE, ITEM_PICKUP, 10, 10, 0, "name", "proxMsg", "useMsg", "detail", {0,0,NULL} },

    { ITEM_WEAPON, ITEM_RIFLE, ITEM_PICKUP, 10, 10, 0, "a Lee-Enfield rifle", "a rifle", "probably not a good idea to do that here", "The Lee-Enfield rifle was first introduced to the United States military in 1906 and is still in use today. Its classic bolt action was called by General Douglas McArthur \"the most deadly killing machine known to man.\" It fires .303 centimeter ammunition.", {0,0,NULL} },
    { ITEM_WEAPON, ITEM_BASEBALL_BAT, ITEM_PICKUP, 3, 5, 0, "a baseball bat", "baseball bat", "hey, save it for the arena, slugger", "A Louisville slugger; a solid hunk of wood, ought to do some damage", {0,0,NULL} },

    // TOOLS
    { ITEM_TOOL, ITEM_TEMPLATE, ITEM_PICKUP, 100, 15, 0, "name", "proxMsg", "useMsg", "detail", {0,0,NULL} },

	{ ITEM_TOOL, ITEM_PLIERS, ITEM_PICKUP, 1, 1, 0, "pliers", "a pliers", "you attempt to loosen some bolts. better quit it before you break something.", "a regular, run-of-the-mill hardware store pliers", _nullEffect },
    { ITEM_TOOL, ITEM_GASCAN, ITEM_PICKUP, 5, 10, 1, "gas can", "a gas can", "nah, better not", "a regular old 2 gallon can of gas (full)", {0,0,NULL} },
    { ITEM_TOOL, ITEM_303AMMO, ITEM_INSTANT, 15, 0, 1, "303 ammo", "303 ammo", NULL, NULL, {6,5,"player got clip of 303 ammo"} },
    { ITEM_TOOL, ITEM_LOCKBOX, ITEM_PICKUP, 3, 18, 0, "metal lockbox", "a metal lockbox", "it's not good for much since you don't have the key", "a grey metal lockbox. It seems to be in pretty good shape, but you can't seem to find the key that goes with it", {0,0,NULL} },

    // FOOD
    { ITEM_FOOD, ITEM_TEMPLATE, ITEM_PICKUP, 10, 1, 1, "name", "proxMsg", "useMsg", "detail", {3,0,NULL} },

    { ITEM_FOOD, ITEM_BEANS, ITEM_PICKUP, 5, 1, 1, "beans", "a can of beans", "useMsg", "detail", {3,5,"them were some mighty fine beans"} },
    { ITEM_FOOD, ITEM_TUNA, ITEM_PICKUP, 5, 1, 1, "tuna", "a can of tuna", "useMsg", "detail", {3,3,"chicken of the sea donchaknow"} },
    { ITEM_FOOD, ITEM_BOX_OF_WINE, ITEM_PICKUP, 6, 2, 4, "box of wine", "a box of wine", "you drink some wine from the box", "a regular old box of wine.", {3,3,"not the best wine there is, but it's better than nothing"} },
    { ITEM_FOOD, ITEM_SARDINES, ITEM_PICKUP, 8, 1, 1, "sardines", "a tin of sardines", NULL, "a can of fine sardines", {3, 5, "those omega-3s are good for you, yum"} },
    { ITEM_FOOD, ITEM_CHOCOLATE, ITEM_PICKUP, 4, 1, 1, "dark chocolate", "a bar of dark chocolate", "dark chocolate is just soo good.", "an expensive bar of dark chocolate", {3,4,NULL} },
    { ITEM_FOOD, ITEM_COFFEE, ITEM_PICKUP, 5, 1, 1, "coffee beans", "a large bag of coffee beans", "ahhh, there's nothing like a good cup of coffee to cure what ails you", "a large bag of expensive hand-roasted coffee beans. the label says they're from Sumatra.", {3,4,NULL} },
    { ITEM_FOOD, ITEM_VODKA, ITEM_PICKUP, 3, 1, 4, "vodka", "a bottle of Russian vodka", "you drink back a large dram of vodka, wow that packs a kick", "Stolichnaya vodka.", {3,2,NULL} },
    { ITEM_FOOD, ITEM_IBUPROFIN, ITEM_PICKUP, 1, 1, 4, "ibuprofin", "a bottle of ibuprofin", "you eat a handful of 200mg ibuprofin tablets. it only works if you've got a headache", "a bottle of a 1000 ibuprofin", {3,0,NULL} },
    { ITEM_FOOD, ITEM_TEQUILA, ITEM_PICKUP, 3, 1, 4, "tequila", "a bottle of tequila", "you drink deeply from the tequila bottle", "an expensive bottle of Mexican 100%% agave Tequila", {3,2,NULL} },
    { ITEM_FOOD, ITEM_SALAMI, ITEM_PICKUP, 3, 1, 1, "Salami", "an Italian Salami", "You take a big bite of salami, salty goodness.", "a log of good Italian Salami", {3,7,NULL} },
    { ITEM_FOOD, ITEM_RAVIOLI, ITEM_PICKUP, 3, 1, 1, "ravioli", "a can of ravioli", "you eat the can of ravioli. it was good.", "a can of ravioli. probably last a hundred years", {3,2,NULL} },
    { ITEM_FOOD, ITEM_CRACKERS, ITEM_PICKUP, 3, 1, 1, "crackers", "a can of ravioli", "you eat the can of ravioli. it was good.", "a can of ravioli. probably last a hundred years", {3,2,NULL} },
    { ITEM_FOOD, ITEM_CHIPS, ITEM_INSTANT, 2, 1, 1, "chips", "a bag of chips", NULL, NULL, {3,1,"you scarf a bag of chips"} },
    { ITEM_FOOD, ITEM_MARIJUANA, ITEM_PICKUP, 20, 0, 4, "marijuana", "a bag of kindbuds", "You smoke a joint to calm your nerves.  Nothin but existin...", "a bag of kindbuds", {3,2,NULL} },
    { ITEM_FOOD, ITEM_SCOTCH, ITEM_PICKUP, 10, 1, 4, "scotch", "a bottle of good scotch", "you take a swig of scotch like a man", "a bottle of good scotch", {3,3,NULL} },
    { ITEM_FOOD, ITEM_BEER, ITEM_PICKUP, 4, 1, 2, "beer", "a case of beer", "You slam some beer.  Burrrrrp ... better", "a case of beer", {3,2,NULL} },
    { ITEM_FOOD, ITEM_SALMON, ITEM_PICKUP, 8, 1, 1, "salmon", "some smoked salmon", "you wolf down a package of smoked salmon. delicious!", "smoked salmon", {3,8,NULL} },
    { ITEM_FOOD, ITEM_PISTACIOS, ITEM_INSTANT, 2, 1, 1, "pistacios", "some pistacios", "you eat some pistacios quick", "a bag of pistacios", {3,3,NULL } },


/* QUESTIONS: is proxMsg what it prints when you are close to it? why do we
    even need that? we can make one.
    
    ANSER: proxMsg will just be the long description, where name will be the short.

    is useMessage what prints when we use it in inventory? I guess it has to
    be because we dont/cant use items when they are still on the map, we can
    only walk over them and collect them.
    
    Anser: answered already, inventory

    well then, why do we have a itemEffect for use, with a place to hold a message?  as well as useMessage?

    yeah, there's 2 of the same field. pick one, get rid of the other.
*/
};
//#define TOTAL_ITEM_DEFS (sizeof(itemDefsTable)/sizeof(itemDefsTable[0]))

const unsigned int _totalItemDefs = (sizeof(itemDefsTable)/sizeof(itemDefsTable[0]));
