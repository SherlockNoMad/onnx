// Copyright (c) ONNX Project Contributors.
// Licensed under the MIT license.

#include "onnx/defs/schema.h"

namespace ONNX_NAMESPACE {
static const char* Adagrad_ver11_doc = R"DOC(
    Compute one iteration of ADAGRAD, a stochastic gradient based optimization
    algorithm. This operator can conduct the optimization of multiple tensor variables.

    Let's define the behavior of this operator. As you can imagine, ADAGRAD requires
    some parameters:
     
     - The initial learning-rate "R".
     - The update count "T". That is, the number of training iterations conducted.
     - A L2-norm regularization coefficient "norm_coefficient".
     - A learning-rate decay factor "decay_factor".
     - A small constant "epsilon" to avoid dividing-by-zero. 

    At each ADAGRAD iteration, the optimized tensors are moved along a direction
    computed based on their estimated gradient and accumulated squared gradient. Assume
    that only a single tensor "X" is updated by this operator. We need the value of "X",
    its gradient "G", and its accumulated squared gradient "H". Therefore, variables in
    this operator's input list are sequentially "R", "T", "X", "G", and "H". Other
    parameters are given as attributes because they are usually constants. Also, the
    corresponding output tensors are the new value of "X" (called "X_new"), and then
    the new accumulated squared gradient (called "H_new"). Those outputs are computed
    from the given inputs following the pseudo code below.

    Let "+", "-", "*", and "/" are all element-wise arithmetic operations with
    numpy-style broadcasting support. The pseudo code to compute those outputs is:

      // Compute a scalar learning-rate factor. If X is never updated, T should be 0.
      r = R / (1 + T * decay_factor);

      // Add gradient of 0.5 * norm_coefficient * ||X||_2^2, where ||X||_2 is the 2-norm.
      G_regularized = norm_coefficient * X + G;

      // Compute new accumulated squared gradient.
      H_new = H + G_regularized * G_regularized;

      // Compute the adaptive part of per-coordinate learning rate. Note that Sqrt(...)
      // compute square root element-wisely.
      H_adaptive = Sqrt(H_new) + epsilon

      // Compute the new value of "X".
      X_new = X - r * G_regularized / H_adaptive;

    If one assign this operators to optimize multiple inputs, for example, "X_1" and "X_2", the same
    pseudo code may be extended to handle all tensors jointly. More specifically, we can view "X" as a
    concatenation of "X_1" and "X_2" (of course, their gradient and accumulate gradient should
    be concatenated too) and then just reuse the entire pseudo code.

    Note that ADAGRAD was first proposed in http://jmlr.org/papers/volume12/duchi11a/duchi11a.pdf.
    In that reference paper, this operator is a spacial case of the Figure 1's composite mirror
    descent update.
)DOC";

ONNX_OPERATOR_SET_SCHEMA(
    Adagrad,
    11,
    OpSchema()
        .SetDoc(Adagrad_ver11_doc)
        .Input(0, "R", "The initial learning rate.", "T1")
        .Input(1, "T", "The update count of \"X\". It should be a scalar.", "T2")
        .Input(
            2,
            "inputs",
            "It sequentially contains the current values of optimized tensors and then the "
            "current values of accumulated gradient. For example, if two tensor \"X_1\" and \"X_2\" "
            "are optimized, The input list would be [\"X_1\", \"X_2\", gradient of \"X_1\", "
            "gradient of \"X_2\", accumulated squared gradient of \"X_1\", accumulated squared gradient of \"X_2\"].",
            "T3",
            OpSchema::Variadic,
            false)
        .Output(
            0,
            "outputs",
            "It sequentially contains the new values of optimized tensors and then the new "
            "values of accumulated gradient. For example, if two tensor \"X_1\" and \"X_2\" are "
            "optimized, the output list would be [new value of \"X_1,\" new value of \"X_2\" "
            "new accumulated squared gradient of \"X_1\", new accumulated squared gradient of \"X_2\"].",
            "T2",
            OpSchema::Variadic,
            false)
        .Attr(
            "epsilon",
            "Small scalar to avoid dividing by zero.",
            AttributeProto::FLOAT,
            1e-6f)
        .Attr(
            "decay_factor",
            "The decay factor of learning rate after one update."
            "The effective learning rate is computed by r = R / (1 + T * decay_factor). "
            "Default to 0 so that increasing update counts doesn't reduce the learning rate.",
            AttributeProto::FLOAT,
            0.0f)
        .Attr(
            "norm_coefficient",
            "Regularization coefficient in 0.5 * norm_coefficient * ||X||_2^2. Default to 0, "
            "which means no regularization.",
            AttributeProto::FLOAT,
            0.0f)
        .TypeConstraint(
            "T1",
            {"tensor(float)", "tensor(double)"},
            "Constrain input types to float scalars.")
        .TypeConstraint(
            "T2",
            {"tensor(int64)"},
            "Constrain output types to 64-bit integer scalars.")
        .TypeConstraint(
            "T3",
            {"tensor(float)", "tensor(double)"},
            "Constrain input types to float tensors.")
        .TypeAndShapeInferenceFunction([](InferenceContext& ctx) {
            // In comments below, we assume that the input list is
            // [R, T, X1, X2, G1, G2, H1, H2] and the output list is
            // [X1_new, X2_new, H1_new, H2_new].

            // Compute the number of tuples (X, G, H).
            auto num_optimized_tensors = (ctx.getNumInputs() - 2) / 3;
            for (size_t i = 0; i < num_optimized_tensors; ++i) {
              // Pass X1's and X2's shapes to X1_new and X2_new, respectively.
              size_t i_in = 2 + i;
              size_t i_out = i;
              propagateElemTypeFromInputToOutput(ctx, i_in, i_out);
              propagateShapeFromInputToOutput(ctx, i_in, i_out);

              // Pass H1's and H2's shapes to H1_new and H2_new, respectively.
              i_in = 2 + 2 * num_optimized_tensors + i;
              i_out = i + num_optimized_tensors;
              propagateElemTypeFromInputToOutput(ctx, i_in, i_out);
              propagateShapeFromInputToOutput(ctx, i_in, i_out);
            }}));
} // namespace ONNX_NAMESPACE
