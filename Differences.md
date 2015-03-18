# Differences #

The version of Enso that can be downloaded from [The Humanized Website](http://humanized.com/) is known as the "commercial" version of Enso; it is sometimes also called the "frozen" version of Enso.

Commercial Enso is a binary-only distribution, based on a closed-source code-line kept on a private SVN server. Development on it has ceased and will not be resumed. Development-wise, it is a dead end; but it will continue to be the version that users download and install, and Humanized will continue to distribute and support it, until such time as development on the open source Enso code-line creates a suitable replacement.

Commercial Enso runs on Windows 2000, XP, and Vista only.

Open-Source Enso consists of the code tree located at The Enso Project and was started "from scratch" in early 2008. Modules and chunks of code were rapidly brought over from the old code and integrated; this process could be thought of as a combination of code migration and massive refactoring to remove all the cruft that Enso no longer needed, and to make Enso more flexible towards certain things that it needs to support better, such as internationalization and cross-platform support.

Another way to view the difference is that Commercial Enso `embeds` Python, whereas Open-Source Enso attempts to `extend` Python. The difference between the two, and their implications, is nicely summarized in Glyph Lefkowitz's article [Extending vs. Embedding: There is Only One Correct Decision](http://twistedmatrix.com/users/glyph/rant/extendit.html).