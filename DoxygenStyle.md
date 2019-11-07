# Doxygen Documentation Style

Here, we describe how to document the source code.  This represent so sort of minial set of markers that need to be used. Other markers can be used to enhance the documentation of the code.  This serves as a live document that can be updated when needed.

## Comment block

The comment block should use

```
/**
*    ... comment ...
*/
```

## File Documentation

Each file should have a file description.  The keyword is `\file`

```
/** \file file.h
 *  \brief A brief file description.
 * 
 *  A more elaborated file description.
 */
```
The file description should be inserted after the license statement.

## C++ Objects Documentation

1. Classes should have a class comment block with `\class` marker.
2. Functions should have a function comment block with `\fn` marker. 
    * A function should also have `\param`, `\return`, and optionally `\sa`. It is useful to use `[in], [out], [in,out]` to specify the direction of a parameter.
3. Member variables can be documented using 
```
int var; /**< Description after the member */
``` 
