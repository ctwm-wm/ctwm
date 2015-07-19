# Style Guidelines


## Overall

Most of these rules are meant to be general guidelines.  The overriding
goal is for the code to *work*; if it doesn't do that, it's worthless.
Given that it works, it should be *readable*.  These guidelines are aimed
at achieving that goal; if you have to break a rule to be more readable,
then do it.


## Automatic

[Artistic Style](http://astyle.sourceforge.net/) is used to maintain
gross code styling.  The `make indent` target will run it over the
codebase, and should be used regularly to keep things in shape.

These are thus hard rules; in theory, at any given time, running `make
indent` should yield no changes.  This is the primary exception to the
"break the rules" guideline above.  Code should always follow these
rules, because it makes life simpler.


## Include files and ordering

* All source files should include `ctwm.h`, and always include it first.

* Includes should be generally ordered as:

    * `ctwm.h`

    * (some vertical whitespace separator)

    * System includes (stdio, X11/foo, etc)

    * (some vertical whitespace separator)

    * Other local includes

    * However some special cases exist where we have to pull system files
        after the locals; e.g., when something that comes out of our
        config (pulled in via ctwm.h) is needed to figure what or where
        to include.  That's fine.

* Generally, local includes should avoid `#include`ing system includes
    where possible.  `ctwm.h` is a broad exception, as it pulls in large
    numbers of system includes that are widely used.  And special cases
    break this with some regularity.  But if you seem to need to do so,
    take a step back first and see whether it's better done in the
    including `.c` file.



{>>
 vim:expandtab:ft=markdown:formatoptions-=q:formatoptions+=2
<<}
