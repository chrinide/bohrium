import util

class test_reduce_views:
    """ Test reduction of all kind of views"""
    def init(self):
        for cmd, shape in util.gen_random_arrays("R", 4, dtype="np.float32"):
            cmd = "R = bh.random.RandomState(42); a = %s; " % cmd
            for i in range(len(shape)):
                yield (cmd, i)
            for i in range(len(shape)):
                yield (cmd, -i)

    def test_reduce(self, arg):
        (cmd, axis) = arg
        cmd += "res = M.add.reduce(a, axis=%d)" % axis
        return cmd


class test_reduce_sum:
    """ Test reduction of sum(), prod(), any(), and all()"""
    def init(self):
        for cmd, shape in util.gen_random_arrays("R", 3, dtype="np.float32"):
            cmd = "R = bh.random.RandomState(42); a = %s; " % cmd
            for op in ["sum", "prod", "all", "any"]:
                for axis in range(len(shape)):
                    yield (cmd, op, axis)

    def test_func(self, arg):
        (cmd, op, axis) = arg
        cmd += "res = M.%s(a, axis=%d)" % (op, axis)
        return cmd

    def test_method(self, arg):
        (cmd, op, axis) = arg
        cmd += "res = a.%s(axis=%d)" % (op, axis)
        return cmd


class test_reduce_primitives:
    def init(self):
        for op in ["add", "multiply"]:
            yield (op, "np.float64")
            yield (op, "np.complex128")

        for op in ["add", "multiply", "minimum", "maximum"]:
            for dtype in util.TYPES.NORMAL:
                yield (op, dtype)

        for op in ["bitwise_or", "bitwise_xor"]:
            yield (op, "np.uint64")

        for op in ["add", "logical_or", "logical_and", "logical_xor"]:
            yield (op, "np.bool")

    def test_vector(self, arg):
        (op, dtype) = arg
        cmd = "R = bh.random.RandomState(42); a = R.random(10, dtype=%s, bohrium=BH); " % dtype
        cmd += "res = M.%s.reduce(a)" % op
        return cmd
