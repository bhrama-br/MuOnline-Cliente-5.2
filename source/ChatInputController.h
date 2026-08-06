#pragma once

// Shared input state for the legacy chat UI.  Platforms only translate their
// native events into actions; the chat window retains ownership of opening,
// closing and protocol submission.
namespace SEASON3B
{
    enum class ChatInputAction
    {
        None,
        Open,
        Submit,
        Cancel,
    };

    class ChatInputController
    {
    public:
        ChatInputController() : m_openRequested(false), m_submitRequested(false) {}

        void Reset()
        {
            m_openRequested = false;
            m_submitRequested = false;
        }

        void RequestOpen()   { m_openRequested = true; }
        void RequestSubmit() { m_submitRequested = true; }

        ChatInputAction Consume(bool visible, bool textHasFocus,
                                bool legacyEnter, bool legacyEscape)
        {
            if (!visible)
            {
                const bool open = m_openRequested || legacyEnter;
                m_openRequested = false;
                m_submitRequested = false;
                return open ? ChatInputAction::Open : ChatInputAction::None;
            }

            // An open request received after the window is already visible is
            // stale (for example, focus changed between browser callbacks).
            m_openRequested = false;

            if (m_submitRequested || (textHasFocus && legacyEnter))
            {
                m_submitRequested = false;
                return ChatInputAction::Submit;
            }

            if (legacyEscape)
                return ChatInputAction::Cancel;

            return ChatInputAction::None;
        }

    private:
        bool m_openRequested;
        bool m_submitRequested;
    };
}
