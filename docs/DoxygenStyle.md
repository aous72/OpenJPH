# Doxygen Documentation Style

Documentation is still experimental for me, and I might change things down the line.

Here, we describe how to document the source code. This represent so sort of minial set of markers that need to be used. Other markers can be used to enhance the documentation of the code. This serves as a live document that can be updated when needed.
I am preferencing `@` over `\`.

## Comment block

The comment block should use

```
/**
 *    ... comment ...
 */
```

## File Documentation

Each file should have a file description.  The marker `@file` can be used

```
/** @file file.h
 *  @brief A brief file description.
 * 
 *  A more elaborated file description.
 */
```

The file description should be inserted after the license statement.

## C++ Objects Documentation

1. The current plan is to put interface documentation in the .h file,
   and put detailed descriptions in the .cpp files.
2. There is no need to use `@class` marker.
3. There is no need to use a function comment block marker `@fn'`. 
    * A function can also have `@param`, `@return`, and optionally `@sa`. 
    It is useful to use `[in], [out], [in,out]` to specify the direction of 
    a parameter.
4. For template functions, one can use '@tparam'.
5. Member variables can be documented as follows 
    * short comment

        ```
        int var; //!<one line description
        ````