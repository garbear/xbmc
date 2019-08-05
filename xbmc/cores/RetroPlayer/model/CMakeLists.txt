set(SOURCES Test.cpp
)

set(HEADERS achievements/Achievement.h
            achievements/Condition.h
            achievements/Constant.h
            achievements/Expression.h
            achievements/LeaderBoard.h
            achievements/Term.h
            achievements/Variable.h
            file/File.h
            media/Game.h
            summarization/Annotation.h
            summarization/Concept.h
            summarization/Instance.h
            summarization/LanguageString.h
            summarization/Molecule.h
            summarization/Phrase.h
            summarization/Summary.h
)

core_add_library(retroplayer_model)
