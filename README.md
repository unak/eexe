e.exe
=====

What's this?
------------

A tool to invoke an editor for Windows.

+ e.exe can redirect the result of another command from the Command Prompt, and pass it to the editor via a temporary file.
+ e.exe can pass the contents in the screen buffer of the Command Prompt to the editor via a temporary file.
+ Of course, e.exe can open a file with the editor.


Initial setting
---------------

e.exe invokes the application associated with the .txt filetype.
So, you have to associate your favorite editor to the .txt filetype.
And you have to put e.exe in a location included in your Path environment variable.


How to use?
-----------

Run e.exe from Command Prompt.

When running e.exe without any parameters, it saves the contents in the screen buffer to a temporary file, and opens the file with the editor.

```
C:\> e
(opens your editor with a temporary file containing the screen buffer)
```

And, when e.exe is invoked as the target of redirection, it reads the contents from pipe, saves it to a temporary file, and opens the file with the editor.

```
C:\> dir | e
(opens your editor with a temporary file contains the result of `dir` command)
```

These temporary files will be removed right after the editor is invoked (and e.exe will end immediatelty.)
If you want to delay removing the files, pass the `--wait` switch to e.exe.
If the switch is passed, e.exe will wait for the termination of the editor.
These temporary files are created in the current directory.

If you want to open an existing / new file, simply specify the file name as a parameter to e.exe.
You can add the `--wait` switch in this case, too.
Of course, in this case, the file will not be removed automatically.

```
C:\> e foo.txt
(opens your editor with `foo.txt`)

C:\> e --wait foo.txt
(opens your editor with `foo.txt', and waits until the file is closed)
```

If the output of e.exe is also redirected, the content of the edited file is passed to the pipe output.
In such a case, e.exe automatically waits for file closing just as if `--wait` was specified.

```
C:\> dir | e | findstr $
(opens your editor with a temporary file, waits until the file is closed, and passes the content of the file to the `findstr` command)

```

License
-------

Copyright (c) 1998 by U.Nakamura
All rights reserved.

Redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following
conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
