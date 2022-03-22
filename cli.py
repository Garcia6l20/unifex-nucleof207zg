import click
import serial
import serial.tools.list_ports

pass_serial = click.make_pass_decorator(serial.Serial)


@click.group()
@click.pass_context
def cli(ctx):
    com_port = [p.device for p in serial.tools.list_ports.comports()
                if p.pid == 4242][0]
    if com_port is None:
        click.secho('Device not found', fg='red')
        return -1
    ctx.obj = serial.Serial(com_port, baudrate=115200)


@cli.command()
@click.argument('ENABLE')
@pass_serial
def set_led(com: serial.Serial, enable: str):
    com.write(f'set-led {enable}\r\n'.encode())
    click.echo(com.readline().decode()[:-2])


@cli.command()
@click.argument('VALUE')
@pass_serial
def red_delay(com: serial.Serial, value: str):
    com.write(f'red-delay {value}\r\n'.encode())
    click.echo(com.readline().decode()[:-2])


@cli.command()
@click.argument('TEXT')
@pass_serial
def echo(com: serial.Serial, text: str):
    com.write(f'echo {text}\r\n'.encode())
    click.echo(com.readline().decode()[:-2])


if __name__ == '__main__':
    cli()
