module: dylan-user

define library debugger
  use dylan;
  use common-dylan;
  use io;
  use melange-support;
end library debugger;

define module debugger
  use dylan;
  use common-dylan, exclude: { \without-bounds-checks};
  use format;
  use format-out;
  use magic;
  use introspection;
  use system;
  use melange-support;
end module debugger;