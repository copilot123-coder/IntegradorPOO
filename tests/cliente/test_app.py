import unittest
from unittest.mock import Mock, patch
import sys
from io import StringIO

# Import the client class
from cliente.robot_rpc import ClienteRobotRPC


class TestClienteRobotRPC(unittest.TestCase):
    @patch('xmlrpc.client.ServerProxy')
    def test_login_and_reporte_usuario(self, mock_serverproxy):
        # Setup mock server and methods
        mock_server = Mock()
        # Mock system.listMethods to simulate connection
        mock_server.system.listMethods.return_value = ['Login', 'ReporteUsuario']

        # Mock Login response
        mock_server.Login.return_value = {
            'exito': True,
            'sessionId': 'ABC123',
            'tipoUsuario': 'normal',
            'mensaje': 'OK'
        }

        # Mock ReporteUsuario response
        mock_server.ReporteUsuario.return_value = {
            'exito': True,
            'usuario': 'testuser',
            'tiempoConexion': '00:10:00',
            'estadoRobot': 'idle',
            'posicionActual': 'X=0,Y=0,Z=0',
            'comandosEjecutados': '5',
            'comandosErroneos': '0',
            'reporteGeneral': 'total_ordenes,0\nordenes_erroneas,0\n'
        }

        mock_serverproxy.return_value = mock_server

        # Capture stdout
        old_stdout = sys.stdout
        sys.stdout = StringIO()

        # Create client and perform login
        cliente = ClienteRobotRPC('localhost', 8080)
        login_ok = cliente.login('user', 'pass')
        self.assertTrue(login_ok)
        self.assertEqual(cliente.session_id, 'ABC123')
        self.assertEqual(cliente.tipo_usuario, 'normal')

        # Call reporte_usuario and verify it returns True
        reporte_ok = cliente.reporte_usuario()
        self.assertTrue(reporte_ok)

        output = sys.stdout.getvalue()
        # Check that login and reporte printed success messages
        self.assertIn('Login exitoso', output)
        self.assertIn('REPORTE DE ACTIVIDAD PERSONAL', output.replace('\n', ' '))

        # Restore stdout
        sys.stdout = old_stdout


if __name__ == '__main__':
    unittest.main()
