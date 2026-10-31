// Groove Grabber - kit names.
//
// Every build gets four fresh names, made from the genre and style, the words you typed (places, seasons,
// moods: "westcoast", "rainy", "late night") and each kit's own character - the darkest kit gets the dark
// name, the dustiest the dusty one: "Midnight Lowrider", "Southside Knock", "Faded Crate Slap".
#pragma once

#include "Kit.h"

namespace gg
{
    // Names every kit in the set (all different, no word used twice across the set)
    void nameKits (std::vector<Kit>& kits, const juce::String& description, juce::uint32 seed);

    // The words that name places and moods in a description ("rainy westcoast night" -> Rainy, Westside, Late Night)
    juce::StringArray nameWordsIn (const juce::String& description);
}
