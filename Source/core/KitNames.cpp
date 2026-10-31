#include "KitNames.h"
#include "References.h"
#include "Styles.h"
#include <iterator>
#include <set>

namespace gg
{
    namespace
    {
        //==============================================================================
        // Words per style: scenes (where it lives) and things (what it's made of). A style also uses
        // its genre's general words.
        struct Vocab { const char* id; const char* scenes; const char* things; };
        static const Vocab vocab[] = {
            { "hiphop",           "Block Party|Basement|Cypher|Corner Store|Rooftop|Uptown",            "Boombox|Crate|Concrete|Mic Check|Breakbeat|Graffiti" },
            { "hiphop.east",      "Brooklyn|Bronx|Queensbridge|Subway|Brownstone|Harlem",               "Timbs|Crate|Concrete|Fire Escape|Break|Cipher" },
            { "hiphop.west",      "Westside|Crenshaw|Long Beach|Compton|Boulevard|Pacific",              "Lowrider|Six-Four|Palm Tree|Hydraulics|Sunset|Bounce" },
            { "hiphop.memphis",   "Memphis|Southside|Midnight Drive|Back Road|Bluff City",               "Cowbell|Tape Deck|Phonk|Cassette|Fog|Drift" },
            { "hiphop.south",     "Dirty South|Southside|Bayou|H-Town|Magnolia",                         "Crunk|Candy Paint|Swang|Slab|Bass Car" },
            { "hiphop.oldschool", "Bronx|Block Party|Roller Rink|Rec Center|Breakdance",                 "Boombox|Electro|Turntable|Scratch|Cassette|Graffiti" },
            { "trap",             "Trenches|Southside|Penthouse|Night Shift|Block|Traphouse",            "Ice|Drip|Chains|Vault|Wave|Sauce|Smoke|Bag" },
            { "trap.atlanta",     "ATL|Zone 6|Peachtree|Southside|Eastside",                             "Ice|Drip|Bando|Chains|Rollie|Bankroll" },
            { "trap.ukdrill",     "London|Ends|Night Bus|Estate|Fog Town|Postcode",                       "Slide|Gloom|Rain|Grey Skies|Hoodie|Bus Stop" },
            { "trap.nydrill",     "Brooklyn|Bronx|Five Boroughs|Subway|Uptown",                          "Slide|Concrete|Timbs|Winter Coat|Dance" },
            { "trap.rage",        "Mosh Pit|Afterparty|Warehouse|Arena|Underground",                     "Rage|Static|Distortion|Laser|Voltage|Chaos" },
            { "trap.plugg",       "Cloud Nine|Daydream|Pink Sky|Sunroof|Bedroom",                        "Bells|Glow|Pluck|Starlight|Haze|Candy" },
            { "rnb",              "After Hours|Candlelight|Penthouse|Slow Lane|Midnight",                 "Velvet|Silk|Satin|Slow Jam|Pillow Talk|Rose" },
            { "rnb.newjack",      "Rooftop|Mall|Block|Prom Night|Uptown",                                "New Jack|Swing|Pager|Fresh Fit|Shoulder Pads" },
            { "rnb.neosoul",      "Brownstone|Lounge|Sunday Morning|Jazz Club|Living Room",              "Incense|Vinyl|Honey|Rhodes|Velvet|Tea" },
            { "rnb.alt",          "After Hours|Night Drive|Skyline|Rooftop|Afterglow",                   "Haze|Neon|Satin|Echo|Smoke|Glass" },
            { "rnb.funk",         "Motor City|Soul Train|Dance Floor|Studio A|Uptown",                    "Groove|Bell Bottoms|Horn Section|Strut|Funk|Shuffle" },
            { "pop",              "Spotlight|Radio|Main Stage|Mall|Rooftop",                             "Sugar|Glitter|Prism|Daydream|Hook|Bubblegum" },
            { "pop.synth",        "Arcade|Night Drive|Miami|Neon City|Rollerdisco",                       "Synthwave|VHS|Laser|Chrome|Cassette|Sunglasses" },
            { "pop.dance",        "Dance Floor|Club|Main Stage|Ibiza|Afterparty",                         "Strobe|Glitter|Pulse|Disco Ball|Confetti" },
            { "pop.rock",         "Garage|Main Stage|Highway|Summer Tour|Backyard",                       "Anthem|Power Chord|Sing-Along|Fireworks|Varsity" },
            { "pop.indie",        "Bedroom|Basement|Polaroid|Thrift Store|Suburb",                        "Cassette|Daydream|Fuzz|Sweater|Postcard|Lo-Fi" },
            { "rock",             "Garage|Stadium|Highway|Dive Bar|Main Stage",                           "Thunder|Amp Stack|Live Wire|Riff|Sweat|Leather" },
            { "rock.classic",     "Stadium|Highway|Desert|Woodstock|Arena",                               "Thunder|Leather|Vinyl|Fringe|Anthem|Stack" },
            { "rock.hard",        "Arena|Stadium|Sunset Strip|Highway|Backstage",                         "Hammer|Thunder|Steel|Leather|Hard Hitter|Overdrive" },
            { "rock.metal",       "Pit|Iron Gate|Forge|Abyss|Black Forest",                               "Iron|Steel|Hammer|Inferno|Anvil|Blast" },
            { "rock.alt",         "Basement|Seattle|Rainy Town|Thrift Store|Garage",                      "Grunge|Flannel|Fuzz|Static|Feedback|Teen Spirit" },
            { "rock.punk",        "Skate Park|Basement|Suburb|Garage|Warped Tour",                        "Safety Pin|Mohawk|Fast Lane|Riot|Skateboard|Spray Paint" },
            { "country",          "Back Porch|Dirt Road|Main Street|Barn|Tailgate|Small Town",            "Boots|Dust|Whiskey|Pickup|Hay Bale|Lantern" },
            { "country.classic",  "Honky Tonk|Dance Hall|Jukebox|Saloon|Opry",                            "Two-Step|Rhinestone|Jukebox|Pedal Steel|Lonesome" },
            { "country.modern",   "Nashville|Broadway|Tailgate|Lake|Friday Night",                        "Pickup|Neon Sign|Bonfire|Boots|Sunburn|Fireflies" },
            { "country.rap",      "Backroads|Mud Hole|County Line|Tailgate|Bayou",                        "Mud Tires|Lifted|Camo|Bass Truck|Dirt" },
            { "country.americana","Front Porch|Prairie|Appalachia|Old Barn|Holler",                       "Banjo|Fiddle|Lantern|Timber|Wildflower|Dust" },
            { "worship",          "Sanctuary|Morning Light|Altar|Upper Room|Mountaintop",                 "Grace|Glory|Selah|Hallelujah|Ember|Anthem" },
            { "worship.modern",   "Arena|Sanctuary|Mountaintop|Open Sky|Upper Room",                      "Anthem|Glory|Ember|Hosanna|Wildfire|Crown" },
            { "worship.gospel",   "Sunday|Choir Loft|Tabernacle|Revival|Church House",                    "Praise Break|Shout|Organ|Hallelujah|Tambourine|Glory" },
            { "worship.ambient",  "Cathedral|Stillwater|Dawn|Chapel|Open Field",                          "Hymn|Candle|Selah|Stillness|Light|Breath" },
            { "edm",              "Main Stage|Festival|Warehouse|Afterparty|Laser Room",                  "Voltage|Drop|Strobe|Pulse|Overdrive|Afterglow" },
            { "edm.house",        "Chicago|Ibiza|Dance Floor|Loft|Sunrise Set",                           "Jack|Groove|Disco Ball|Vocal Chop|Diva|Piano House" },
            { "edm.techno",       "Berlin|Warehouse|Bunker|Detroit|Underground",                          "Steel|Machine|Strobe|Concrete|Pulse|Circuit" },
            { "edm.dnb",          "Jungle|Rave|Bristol|Night Bus|Tunnel",                                 "Amen|Rush|Velocity|Roller|Breakneck|Reese" },
            { "edm.dubstep",      "Rave|Bunker|Low End|Underground|Main Stage",                           "Wobble|Growl|Bass Face|Headbanger|Wub|Quake" },
            { "edm.disco",        "Roller Rink|Studio 54|Dance Floor|Mirror Ball|Funk Night",             "Disco Ball|Glitter|Strut|Satin|Boogie|Platform" },
            { "lofi",             "Rainy Window|Study Hall|Late Bus|Bedroom|Corner Cafe",                 "Cassette|Tape Hiss|Matcha|Sweater|Notebook|Polaroid" },
            { "lofi.jazzhop",     "Jazz Cafe|Bookstore|Rooftop|Sunday Morning|Coffee Shop",               "Vinyl|Rhodes|Espresso|Upright|Brushes|Record Crate" },
            { "lofi.trap",        "Bedroom|Rainy Night|Skyline|Late Bus|3 AM",                            "Tears|Static|Hoodie|Moonlight|Tape|Echo" },
            { "cinematic",        "Empire|Battlefield|Canyon|Throne Room|Storm Front",                    "Titan|Eclipse|Monolith|Colossus|Thunder|Requiem" },
            { "cinematic.trailer","Coming Soon|Final Act|Battlefield|Skyfall|Citadel",                    "Impact|Titan|Braam|Countdown|Colossus|Riser" },
            { "cinematic.horror", "Haunted House|Crypt|Basement|Dark Woods|Asylum",                       "Shadow|Whisper|Nightmare|Dread|Creak|Hollow" },
            { "cinematic.world",  "Desert|Jungle|Monsoon|Silk Road|Temple",                               "Taiko|Djembe|Caravan|Dune|Ritual|Earth" },
        };

        //==============================================================================
        // Each kit's character: the words for what stands out about it
        juce::StringArray split (const char* s);
        struct Feature { int trait; bool high; const char* moods; const char* hits; };
        static const Feature features[] = {
            { 0, true,  "Heavy|Heavyweight|Big|Knockout",                         "Knock|Slap|Punch|Thump|Stomp|Bang" },
            { 0, false, "Soft|Easy|Gentle|Lazy",                                  "Sway|Brush|Drift" },
            { 1, true,  "Velvet|Honey|Amber|Warm|Golden Hour",                    "Bounce|Roll|Glow" },
            { 1, false, "Cold|Frost|Icy|Winter",                                  "Snap|Chill" },
            { 2, true,  "Neon|Chrome|Crystal|Platinum|Diamond|Sunlit",            "Snap|Shine|Sparkle" },
            { 2, false, "Midnight|Shadow|Moonlit|Obsidian|After Dark|Low Light",  "Rumble|Boom|Thud" },
            { 3, true,  "Echo|Wide Open|Canyon|Grand|Skyline",                    "Boom|Roar|Echo" },
            { 3, false, "Tight|Dry|Close|Intimate",                               "Snap|Tap" },
            { 4, true,  "Dusty|Faded|Analog|Sepia|Old Soul|Vintage",              "Break|Crackle|Thump" },
            { 4, false, "Fresh|Crisp|Clean|Brand New|Glossy",                     "Snap|Knock" },
            { 5, true,  "Savage|Raw|Riot|Feral|Wild|Rough",                       "Smash|Slam|Stomp|Crunch" },
            { 5, false, "Smooth|Silk|Mellow|Laid-Back|Easy",                      "Sway|Glide|Roll|Bounce" },
            { 6, true,  "Live|Session|Wooden|Handmade|Studio",                    "Groove|Pocket|Shuffle" },
            { 6, false, "Digital|Circuit|Electric|Synthetic",                     "Pulse|Spark|Voltage" },
        };

        // words that don't suit a genre, whatever the kit sounds like
        const juce::StringArray& avoidFor (Genre g)
        {
            static const juce::StringArray worship = split ("Savage|Raw|Riot|Feral|Wild|Rough|Smash|Slam|Crunch|Bang|Slap|Knock|Knockout|Punch|Stomp|Digital|Circuit|Synthetic|Electric|Spark|Voltage|Cold|Icy|Shadow|Obsidian|Lazy|Heavyweight|Chrome|Dry|Fresh|Glossy"),
                                           heavy = split ("Smooth|Silk|Mellow|Laid-Back|Easy|Soft|Gentle|Lazy|Fresh|Glossy|Velvet|Honey|Sway|Glide|Brush|Drift|Sparkle|Brand New|Clean"),
                                           smooth = split ("Savage|Riot|Feral|Smash|Slam|Crunch|Bang|Knockout|Heavyweight"),
                                           film = split ("Slap|Bounce|Sway|Groove|Pocket|Shuffle|Fresh|Glossy|Lazy|Knockout|Brand New"),
                                           country = split ("Digital|Circuit|Synthetic|Electric|Spark|Voltage|Obsidian|Chrome|Platinum|Diamond|Crystal|Glossy"),
                                           none;
            switch (g)
            {
                case Genre::worship:   return worship;
                case Genre::rock:      return heavy;
                case Genre::rnb:
                case Genre::lofi:      return smooth;
                case Genre::cinematic: return film;
                case Genre::country:   return country;
                default:               return none;
            }
        }

        //==============================================================================
        // Places and moods you might type: the phrase, and the word it gives a name
        struct Said { const char* phrase; const char* word; };
        static const Said saidWords[] = {
            { "west coast", "Westside" }, { "westcoast", "Westside" }, { "west side", "Westside" }, { "westside", "Westside" },
            { "east coast", "Eastside" }, { "eastcoast", "Eastside" }, { "east side", "Eastside" }, { "eastside", "Eastside" },
            { "south side", "Southside" }, { "southside", "Southside" }, { "dirty south", "Dirty South" }, { "north side", "Northside" },
            { "cali", "Cali" }, { "california", "Cali" }, { "los angeles", "LA" }, { "la", "LA" }, { "bay area", "Bay Area" },
            { "new york", "New York" }, { "nyc", "New York" }, { "brooklyn", "Brooklyn" }, { "bronx", "Bronx" }, { "harlem", "Harlem" },
            { "atlanta", "ATL" }, { "atl", "ATL" }, { "houston", "H-Town" }, { "memphis", "Memphis" }, { "detroit", "Motor City" },
            { "chicago", "Windy City" }, { "miami", "Miami" }, { "new orleans", "Bayou" }, { "texas", "Lone Star" }, { "nashville", "Nashville" },
            { "london", "London" }, { "uk", "London" }, { "toronto", "The 6" }, { "paris", "Paris" }, { "tokyo", "Tokyo" }, { "berlin", "Berlin" },
            { "summer", "Summer" }, { "winter", "Winter" }, { "spring", "Spring" }, { "autumn", "Autumn" }, { "fall", "Autumn" },
            { "night", "Late Night" }, { "late night", "Late Night" }, { "midnight", "Midnight" }, { "morning", "Morning" },
            { "sunset", "Sunset" }, { "sunrise", "Sunrise" }, { "rain", "Rainy" }, { "rainy", "Rainy" }, { "storm", "Storm" }, { "snow", "Snowfall" },
            { "ocean", "Ocean" }, { "beach", "Beach" }, { "island", "Island" }, { "desert", "Desert" }, { "mountain", "Mountain" },
            { "city", "City" }, { "street", "Street" }, { "streets", "Street" }, { "club", "Club" }, { "party", "Party" }, { "car", "Night Drive" },
            { "drive", "Night Drive" }, { "space", "Cosmic" }, { "galaxy", "Galaxy" }, { "moon", "Moonlight" }, { "dream", "Dream" },
            { "dreamy", "Dreamy" }, { "love", "Lover's" }, { "romantic", "Lover's" }, { "heartbreak", "Heartbreak" }, { "sad", "Blue" },
            { "lonely", "Lonely" }, { "happy", "Sunny" }, { "hype", "Hype" }, { "chill", "Chill" }, { "hood", "Hood" }, { "money", "Bankroll" },
            { "luxury", "Luxury" }, { "gold", "Golden" }, { "fire", "Fire" }, { "ice", "Ice" }, { "smoke", "Smoke" }, { "ghost", "Ghost" },
            { "sunday", "Sunday" }, { "friday", "Friday Night" }, { "weekend", "Weekend" }, { "festival", "Festival" }, { "wedding", "Wedding" },
        };

        juce::StringArray split (const char* s)
        {
            auto a = juce::StringArray::fromTokens (s, "|", "");
            a.trim();
            a.removeEmptyStrings();
            return a;
        }

        const Vocab* vocabFor (const juce::String& id)
        {
            for (auto& v : vocab) if (id == v.id) return &v;
            return nullptr;
        }

        // the words of a name, lower case, one by one ("Golden Hour Knock" -> golden, hour, knock)
        juce::StringArray wordsOf (const juce::String& name)
        {
            auto a = juce::StringArray::fromTokens (name.toLowerCase().replaceCharacter ('-', ' '), " ", "");
            a.removeEmptyStrings();
            return a;
        }

        struct Picker
        {
            juce::Random& rng;
            std::set<juce::String>& used;    // lower-case words already in a name

            bool free (const juce::String& phrase) const
            {
                for (auto& w : wordsOf (phrase))
                    if (w.length() > 2 && used.count (w) > 0) return false;
                return true;
            }

            juce::String from (const juce::StringArray& pool, const juce::String& avoidIn = {})
            {
                juce::StringArray ok;
                for (auto& p : pool)
                {
                    bool clash = ! free (p);
                    for (auto& w : wordsOf (p)) if (wordsOf (avoidIn).contains (w)) clash = true;
                    if (! clash) ok.add (p);
                }
                if (ok.isEmpty()) return {};
                return ok[rng.nextInt (ok.size())];
            }
        };
    }

    //==============================================================================
    juce::StringArray nameWordsIn (const juce::String& description)
    {
        juce::StringArray out;
        if (description.isEmpty()) return out;
        auto text = normaliseText (description);
        // longest phrases first, each part of the text used once ("west coast" before "west")
        std::vector<const Said*> order;
        for (auto& s : saidWords) order.push_back (&s);
        std::stable_sort (order.begin(), order.end(), [] (const Said* a, const Said* b) { return juce::String (a->phrase).length() > juce::String (b->phrase).length(); });
        for (auto* s : order)
        {
            const auto p = " " + juce::String (s->phrase) + " ";
            if (text.contains (p))
            {
                out.addIfNotAlreadyThere (s->word);
                text = text.replace (p, " _ ");
            }
        }
        return out;
    }

    void nameKits (std::vector<Kit>& kits, const juce::String& description, juce::uint32 seed)
    {
        if (kits.empty()) return;
        juce::Random rng ((juce::int64) seed * 7919 + 104729);
        std::set<juce::String> used;
        Picker pick { rng, used };

        // the scene and thing words: the kit's style, its genre, and any other style you named ("trap, westcoast")
        const auto text = normaliseText (description);
        juce::StringArray extraScenes, extraThings;
        for (auto& v : vocab)
        {
            Genre g;
            int sub = 0;
            if (! findStyle (v.id, g, sub) || sub == 0) continue;
            for (auto& kw : styleFor (g, sub).keywords)
                if (text.contains (" " + normaliseText (kw).trim() + " "))
                {
                    extraScenes.addArray (split (v.scenes));
                    extraThings.addArray (split (v.things));
                    break;
                }
        }
        const auto said = nameWordsIn (description);

        // what stands out about each kit, against the other kits and against the middle
        Traits mean;
        for (int t = 0; t < Traits::num; ++t)
        {
            float sum = 0;
            for (auto& k : kits) sum += k.target[t];
            mean[t] = sum / (float) kits.size();
        }
        std::set<int> featuresTaken;
        int saidUsed = 0, hitNames = 0;

        for (size_t i = 0; i < kits.size(); ++i)
        {
            auto& k = kits[i];
            juce::StringArray scenes, things;
            if (auto* v = vocabFor (k.styleId)) { scenes.addArray (split (v->scenes)); things.addArray (split (v->things)); }
            const auto genreId = k.styleId.upToFirstOccurrenceOf (".", false, false);
            if (genreId != k.styleId)
                if (auto* v = vocabFor (genreId)) { scenes.addArray (split (v->scenes)); things.addArray (split (v->things)); }
            if (scenes.isEmpty())
                if (auto* v = vocabFor (styleFor (k.genre).id)) { scenes.addArray (split (v->scenes)); things.addArray (split (v->things)); }
            scenes.addArray (extraScenes);
            things.addArray (extraThings);

            // the strongest feature not already given to another kit
            int best = -1;
            float bestScore = -1;
            for (int f = 0; f < (int) std::size (features); ++f)
            {
                const auto& ft = features[f];
                const float rel = k.target[ft.trait] - mean[ft.trait], abs = k.target[ft.trait] - 0.5f;
                const float score = (ft.high ? 1.0f : -1.0f) * (0.65f * rel + 0.35f * abs) + rng.nextFloat() * 0.02f;
                if (featuresTaken.count (f) == 0 && score > bestScore) { best = f; bestScore = score; }
            }
            if (best >= 0) featuresTaken.insert (best);
            const auto moods = best >= 0 ? split (features[best].moods) : juce::StringArray();
            auto hits = best >= 0 ? split (features[best].hits) : juce::StringArray();
            hits.addArray (split (features[0].hits));    // anything can knock

            // a word you typed goes in the first kits' names
            juce::String first;
            if (saidUsed < said.size() && (i == 0 || rng.nextFloat() < 0.5f))
            {
                const auto w = said[saidUsed++];
                if (pick.free (w)) first = w;
            }

            const auto& avoid = avoidFor (k.genre);
            auto keep = [&avoid] (const juce::StringArray& a) { juce::StringArray out; for (auto& w : a) if (! avoid.contains (w)) out.add (w); return out; };
            const auto moodsOk = keep (moods), hitsOk = keep (hits);

            juce::String name;
            for (int attempt = 0; attempt < 10 && name.isEmpty(); ++attempt)
            {
                juce::String a, b;
                const bool hitAllowed = hitNames < 2 && attempt < 6;
                if (first.isNotEmpty())
                {
                    a = first;
                    const int r = rng.nextInt (3);
                    b = r == 0 ? pick.from (scenes, a) : (r == 1 || ! hitAllowed ? pick.from (things, a) : pick.from (hitsOk, a));
                }
                else
                {
                    const int r = rng.nextInt (10);
                    if (r < 3)                    { a = pick.from (moodsOk); b = pick.from (things, a); }    // Midnight Lowrider
                    else if (r < 5)               { a = pick.from (moodsOk); b = pick.from (scenes, a); }    // Neon Penthouse
                    else if (r < 7)               { a = pick.from (scenes);  b = pick.from (things, a); }    // Crenshaw Six-Four
                    else if (r < 8 && hitAllowed) { a = pick.from (scenes);  b = pick.from (hitsOk, a); }    // Southside Knock
                    else if (r < 9 && hitAllowed) { a = pick.from (things);  b = pick.from (hitsOk, a); }    // Lowrider Bounce
                    else if (hitAllowed)          { a = pick.from (moodsOk); b = pick.from (hitsOk, a); }    // Dusty Knock
                    else                          { a = pick.from (moodsOk); b = pick.from (things, a); }
                }
                if (a.isNotEmpty() && b.isNotEmpty() && pick.free (a) && pick.free (b) && (a + " " + b).length() <= 22)
                {
                    name = a + " " + b;
                    if (hitsOk.contains (b)) ++hitNames;
                }
            }
            if (name.isEmpty())
                name = k.name.isNotEmpty() ? k.name : "Kit " + juce::String ((int) i + 1);

            // the decade, now and then
            if (k.era >= 0 && i == 1 && ! name.containsIgnoreCase (eraName (k.era)) && eraName (k.era) != "Today"
                 && (eraName (k.era) + " " + name).length() <= 24)
                name = eraName (k.era) + " " + name;

            for (auto& w : wordsOf (name)) used.insert (w);
            k.name = name;
        }
    }
}
