subroutine test(N)
    integer, intent(in) :: N
    integer :: i
    real :: a
    
    !$omp target device(0)
    do i = 1, N
        a = 3.14
    end do
    !$omp end target
end subroutine
