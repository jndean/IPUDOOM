
GARPHOBJS = AM_map.gp # m_fixed.gp

all: ipu_rt.gp

ipu_rt.gp: $(GARPHOBJS) ipu_vertices.gp
	popc $(GARPHOBJS) ipu_vertices.gp -o $@

ipu_vertices.gp: ipu_vertices.cpp
	popc -Os $^ -o $@

%.gp: %.c
	popc -Os $^ -o $@

clean:
	rm $(GARPHOBJS) ipu_vertices.gp