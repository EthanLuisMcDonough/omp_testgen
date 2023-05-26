subroutine test()
    integer, allocatable :: x, y
    !$omp allocators allocate(omp_default_mem_alloc: x)
        allocate(x)
    
    !$omp allocators allocate(omp_default_mem_alloc: y)
        allocate(y)
end subroutine