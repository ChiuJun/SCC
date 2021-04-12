//
// Created by LJChi on 2021/3/28.
//

#ifndef SCC_ERROR_H
#define SCC_ERROR_H

void DoFatal(const char *format, ...);
void DoError(Coord current_coord, const char *format, ...);
void DoWarning(Coord current_coord, const char *format, ...);

#define Fatal fprintf(stderr,"%s,%d:",__FILE__,__LINE__),DoFatal
#define Error fprintf(stderr,"%s,%d:",__FILE__,__LINE__),DoError
#define Warning fprintf(stderr,"%s,%d:",__FILE__,__LINE__),DoWarning

#endif //SCC_ERROR_H
