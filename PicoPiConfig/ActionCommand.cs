using System;
using System.Windows.Input;

namespace PicoPiConfig
{
    public class ActionCommand : ICommand
    {
        private Action _action;
        private Func<bool>? _canExecute;

        public event EventHandler? CanExecuteChanged;

        public ActionCommand(Action action, Func<bool>? canExecute = null)
        {
            _action = action;
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => (_canExecute == null) || _canExecute();
        public void Execute(object? parameter) => _action();
    }

    public class ActionCommand<T> : ICommand
    {
        private Action<T> _action;
        private Func<bool>? _canExecute;

        public event EventHandler? CanExecuteChanged;

        public ActionCommand(Action<T> action, Func<bool>? canExecute = null)
        {
            _action = action;
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => (_canExecute == null) || _canExecute();
        public void Execute(object? parameter) => _action((T)parameter!);
    }
}
