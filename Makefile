SUBDIR= test bench

ECL_SRCS=impl.hpp list.hpp tailq.hpp

run: ${SUBDIR}
.for DIR in ${SUBDIR}
	${.OBJDIR}/${DIR}/ecl-${DIR}
.endfor

.include <bsd.subdir.mk>
