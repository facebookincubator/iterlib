from collections import Callable


# https://github.com/mbr/visitor
class Visitor:
    """Base class for visitors."""

    async def visit(self, node):
        """Visit a node.

           Input is assumed to be validated json against a schema.
           Dispatch to a visit_foo if the first element in the json
           is foo.
        """
        if not node:
            return
        if isinstance(node, Callable):
            return self.visit_callable(node)
        name = str(node.OP).lower().split(".")[-1]
        meth = getattr(self, "visit_" + name, None)
        await meth(node)

    async def visit_child(self, child):
        """To be called whenever a node with multiple children
           needs to visit children. Compared to visit(), this can
           do cleanup work that needs to be scheduled after each
           child."""
        await self.visit(child)
        await self.finish()

    async def finish(self):
        """Any cleanup work to be done after visit_child."""
        pass
