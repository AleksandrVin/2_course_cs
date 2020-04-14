program magic_func

    implicit none

    type node
        node, pointer :: next
        node, pointer :: prev
        integer, pointer :: data
    end type node

    type list
        node, pointer :: head
        node, pointer :: tail
        integer size
    end type list
    
    print *, "hi"

end program magic_func
