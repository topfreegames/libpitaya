# API
- Problems with `pc_client_init`
Currently, `pc_client_init` expects a poiter to a `pc_client_t` passed by the user. Inside the function, it this pointer for a specific value on the field `magic_num`. If the field equals of the `magic_num`, it means that the used called `pc_client_init` two times, so it just returns without doing anything. The problem is that since the pointer can be allocated with `malloc`, the contents will be garbage. So it may happen every so often that the field `magic_num` has the correct value without having to call the function twice.

- Distinction between application errors and developer errors
Currently, the api does not make the distinction between the two kind of errors. For example, in the function `pc_client_init`, if the client passes a `NULL` pointer as first parameter, the function returns an error code. The thing is that passing a `NULL` pointer is a programmer error, therefore these things should immediately crash the client, in order to avoid the error leaking to production and to also simplify tests.

# Bugs
- Currently, calling `pc_lib_init` `pc_lib_cleanup` and after `pc_lib_init` crashes the client. It
  seems to be something related to the SSL library. This bug is currently not high priority, since `pc_lib_init` should be called only once.

# Success and Error callbacks in `pc_request_with_timeout`
Currently, there are possibly two error callbacks in the function. The last parameter receives one
error callback function. Before the last one there is a generic callback, that can receive a status
code (i.e. success and failures). A better change would be to make one callback only for errors and
one only for successes.

# Solving things with GYP
## Specifying build types
It is not that obvious how one selects the build type. A solution that we found was to define a variable `build_type` and use it for branching on the specific build.

```
{
  'variables': {
    'build_type%': "Release",
  },
  'target_defaults': {
    'default_configuration': '<(build_type)',
    'configurations': {
      'Debug': {
        ...
      },
      'Release': {
        ...
      }
    }
  }
}
```
