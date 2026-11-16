// Groove Grabber - COACH MY BEAT's report (SAUCE's bottom panel while you're coached): four ratings, then the tips,
// each a bold direct line and why the style does it, with TRY IT (make the change; UNDO takes it back) and
// ON PURPOSE (leave it out from now on), then what's working. Pointing at a tip lights its steps on the grid.
#pragma once

#include "Theme.h"
#include "core/Coach.h"

namespace ggui
{
    class CoachPanel : public juce::Component
    {
    public:
        CoachPanel();
        ~CoachPanel() override;
        void setReport (const gg::coach::Report&);
        void setStale (bool);                       // the beat or the style changed since: coach it again
        bool isStale() const noexcept               { return stale; }
        const gg::coach::Report& getReport() const noexcept   { return report; }
        void paint (juce::Graphics&) override;
        void resized() override;
        void lookAndFeelChanged() override;

        std::function<void (const gg::coach::Tip&)> onTry, onOnPurpose;
        std::function<void (const gg::coach::Tip*)> onHover;      // nullptr: none
        std::function<void()> onBringBack;

        // (for the tests) the fixes' TRY IT and ON PURPOSE, by their place in the report
        void tryTip (int fixIndex);
        void markOnPurpose (int fixIndex);

    private:
        class TipCard;
        class Content;
        void rebuild();
        gg::coach::Report report;
        bool stale = false;
        juce::Rectangle<int> ratingsArea, staleArea;
        juce::Viewport view;
        std::unique_ptr<Content> content;
    };
}
