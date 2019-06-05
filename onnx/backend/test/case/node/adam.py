from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import numpy as np  # type: ignore

import onnx
from ..base import Base
from . import expect

def apply_adam(r, t, x, g, v, h, norm_coefficient, alpha, beta, epsilon):  # type: ignore
    # Add gradient of regularization term.
    g_regularized = norm_coefficient * x + g
    # Update momentum.
    v_new = alpha * v + (1 - alpha) * g_regularized
    # Update second-order momentum.
    h_new = beta * h + (1 - beta) * (g_regularized * g_regularized)
    # Compute element-wise square root.
    h_sqrt = np.sqrt(h_new) + epsilon
    # Adjust learning rate.
    t = t + 1
    r_adjusted = r * np.sqrt(1 - beta**t) / (1 - alpha**t)
    # Apply Adam update rule.
    x_new = x - r_adjusted * (v_new / h_sqrt)
    return x_new, v_new, h_new


class Adam(Base):

    @staticmethod
    def export_adam():  # type: () -> None
        # Define operator attributes.
        norm_coefficient = 0.001
        alpha = 0.95
        beta = 0.1
        epsilon = 1e-7

        # Create operator.
        node = onnx.helper.make_node('Adam',
                                     inputs=['R', 'T', 'X', 'G', 'V', 'H'],
                                     outputs=['X_new', 'V_new', 'H_new'],
                                     norm_coefficient=norm_coefficient,
                                     alpha=alpha,
                                     beta=beta,
                                     epsilon=epsilon
                                     )

        # Define operator inputs.
        r = np.array(0.1, dtype=np.float32)  # scalar
        t = np.array(0, dtype=np.int64)  # scalar
        x = np.array([1.2, 2.8], dtype=np.float32)
        g = np.array([-0.94, -2.5], dtype=np.float32)
        v = np.array([1.7, 3.6], dtype=np.float32)
        h = np.array([0.1, 0.1], dtype=np.float32)

        # Compute expected outputs of Adam.
        x_new, v_new, h_new = apply_adam(r, t, x, g, v, h,
                                         norm_coefficient, alpha, beta,
                                         epsilon)

        # Check results.
        expect(node, inputs=[r, t, x, g, v, h],
               outputs=[x_new, v_new, h_new], name='test_adam')

    @staticmethod
    def export_adam_multiple():  # type: () -> None
        # Define operator attributes.
        norm_coefficient = 0.001
        alpha = 0.95
        beta = 0.85
        epsilon = 1e-2

        node = onnx.helper.make_node('Adam',
                                     inputs=['R', 'T', 'X1', 'X2',
                                             'G1', 'G2', 'V1', 'V2',
                                             'H1', 'H2'],
                                     outputs=['X1_new', 'X2_new',
                                              'V1_new', 'V2_new',
                                              'H1_new', 'H2_new'],
                                     norm_coefficient=norm_coefficient,
                                     alpha=alpha,
                                     beta=beta
                                     )

        # Define operator inputs.
        r = np.array(0.1, dtype=np.float32)  # scalar
        t = np.array(0, dtype=np.int64)  # scalar

        x1 = np.array([1.0], dtype=np.float32)
        g1 = np.array([-1.0], dtype=np.float32)
        v1 = np.array([2.0], dtype=np.float32)
        h1 = np.array([0.5], dtype=np.float32)

        x2 = np.array([1.0, 2.0], dtype=np.float32)
        g2 = np.array([-1.0, -3.0], dtype=np.float32)
        v2 = np.array([4.0, 1.0], dtype=np.float32)
        h2 = np.array([1.0], dtype=np.float32)

        # Compute expected outputs of Adam.
        x1_new, v1_new, h1_new = apply_adam(r, t, x1, g1, v1, h1,
                                    norm_coefficient, alpha, beta,
                                    epsilon)
        x2_new, v2_new, h2_new = apply_adam(r, t, x2, g2, v2, h2,
                                    norm_coefficient, alpha, beta,
                                    epsilon)

        # Check results.
        expect(node, inputs=[r, t, x1, x2, g1, g2, v1, v2, h1, h2],
               outputs=[x1_new, x2_new, v1_new, v2_new, h1_new, h2_new],
               name='test_adam_multiple')
